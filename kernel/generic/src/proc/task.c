/*
 * Copyright (c) 2010 Jakub Jermar
 * Copyright (c) 2018 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_generic_proc
 * @{
 */

/**
 * @file
 * @brief Task management.
 */

#include <assert.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <mm/mem.h>
#include <atomic.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch.h>
#include <barrier.h>
#include <adt/list.h>
#include <adt/odict.h>
#include <cap/cap.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <ipc/event.h>
#include <stdio.h>
#include <errno.h>
#include <halt.h>
#include <str.h>
#include <syscall/copy.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <main/uinit.h>

static void task_destroy(void *);

const kobj_class_t kobj_class_task = {
	.destroy = task_destroy,
};

/** Spinlock protecting the @c tasks ordered dictionary. */
IRQ_SPINLOCK_INITIALIZE(tasks_lock);

/** Ordered dictionary of active tasks by task ID.
 *
 * Members are task_t structures.
 *
 * The task is guaranteed to exist after it was found in the @c tasks
 * dictionary as long as:
 *
 * @li the tasks_lock is held,
 * @li the task's lock is held when task's lock is acquired before releasing
 *     tasks_lock or
 * @li the task's refcount is greater than 0
 *
 */
odict_t tasks;

static task_id_t task_counter = 0;

static slab_cache_t *task_cache;

/* Forward declarations. */
static void task_kill_internal(task_t *, int);
static errno_t tsk_constructor(void *, unsigned int);
static size_t tsk_destructor(void *);

static void *tasks_getkey(odlink_t *);
static int tasks_cmp(void *, void *);

/** Initialize kernel tasks support.
 *
 */
void task_init(void)
{
	TASK = NULL;
	odict_initialize(&tasks, tasks_getkey, tasks_cmp);
	task_cache = slab_cache_create("task_t", sizeof(task_t), 0,
	    tsk_constructor, tsk_destructor, 0);
}

/** Kill all tasks except the current task.
 *
 */
void task_done(void)
{
	size_t tasks_left;
	task_t *task;

	if (ipc_box_0) {
		task_t *task_0 = ipc_box_0->task;
		ipc_box_0 = NULL;
		/*
		 * The first task is held by kinit(), we need to release it or
		 * it will never finish cleanup.
		 */
		task_release(task_0);
	}

	/* Repeat until there are any tasks except TASK */
	do {
#ifdef CONFIG_DEBUG
		printf("Killing tasks... ");
#endif
		irq_spinlock_lock(&tasks_lock, true);
		tasks_left = 0;

		task = task_first();
		while (task != NULL) {
			if (task != TASK) {
				tasks_left++;
#ifdef CONFIG_DEBUG
				printf("[%" PRIu64 "] ", task->taskid);
#endif
				task_kill_internal(task, -1);
			}

			task = task_next(task);
		}

		irq_spinlock_unlock(&tasks_lock, true);

		thread_sleep(1);

#ifdef CONFIG_DEBUG
		printf("\n");
#endif
	} while (tasks_left > 0);
}

errno_t tsk_constructor(void *obj, unsigned int kmflags)
{
	task_t *task = (task_t *) obj;

	errno_t rc = caps_task_alloc(task);
	if (rc != EOK)
		return rc;

	rc = kobj_table_initialize(&task->kobj_table);
	if (rc != EOK) {
		caps_task_free(task);
		return rc;
	}

	atomic_store(&task->lifecount, 0);

	irq_spinlock_initialize(&task->lock, "task_t_lock");

	list_initialize(&task->threads);

	ipc_answerbox_init(&task->answerbox, task);

	spinlock_initialize(&task->active_calls_lock, "active_calls_lock");
	list_initialize(&task->active_calls);

#ifdef CONFIG_UDEBUG
	/* Init kbox stuff */
	task->kb.thread = NULL;
	ipc_answerbox_init(&task->kb.box, task);
	mutex_initialize(&task->kb.cleanup_lock, MUTEX_PASSIVE);
#endif

	return EOK;
}

size_t tsk_destructor(void *obj)
{
	task_t *task = (task_t *) obj;

	caps_task_free(task);
	kobj_table_destroy(&task->kobj_table);
	return 0;
}

/** Create new task with no threads.
 *
 * @param as   Task's address space (consumed on success).
 * @param name Symbolic name (a copy is made).
 *
 * @return New task's structure.
 *
 */
task_t *task_create(as_t *as, const char *name)
{
	task_t *task = (task_t *) slab_alloc(task_cache, FRAME_ATOMIC);
	if (!task)
		return NULL;

	kobj_initialize(&task->kobj, KOBJ_CLASS_TASK);

	task_create_arch(task);

	task->as = as;
	str_cpy(task->name, TASK_NAME_BUFLEN, name);

	task->container = CONTAINER;
	task->perms = 0;
	task->ucycles = 0;
	task->kcycles = 0;

	caps_task_init(task);

	task->ipc_info.call_sent = 0;
	task->ipc_info.call_received = 0;
	task->ipc_info.answer_sent = 0;
	task->ipc_info.answer_received = 0;
	task->ipc_info.irq_notif_received = 0;
	task->ipc_info.forwarded = 0;

	event_task_init(task);

	task->answerbox.active = true;

	task->debug_sections = NULL;

#ifdef CONFIG_UDEBUG
	/* Init debugging stuff */
	udebug_task_init(&task->udebug);

	/* Init kbox stuff */
	task->kb.box.active = true;
	task->kb.finished = false;
#endif

	if ((ipc_box_0) &&
	    (container_check(ipc_box_0->task->container, task->container))) {
		cap_phone_handle_t phone_handle;
		errno_t rc = phone_alloc(task, true, &phone_handle, NULL);
		if (rc != EOK) {
			task->as = NULL;
			task_destroy_arch(task);
			slab_free(task_cache, task);
			return NULL;
		}

		kobject_t *phone_obj = kobject_get(task, phone_handle,
		    KOBJECT_TYPE_PHONE);
		(void) ipc_phone_connect(phone_obj->phone, ipc_box_0);
	}

	//waitq_initialize(&task->join_wq);

	irq_spinlock_lock(&tasks_lock, true);

	task->taskid = ++task_counter;
	odlink_initialize(&task->ltasks);
	odict_insert(&task->ltasks, &tasks, NULL);

	irq_spinlock_unlock(&tasks_lock, true);

	return task;
}

/** Destroy task.
 *
 * @param task Task to be destroyed.
 *
 */
static void task_destroy(void *arg)
{
	task_t *task = arg;

	/*
	 * Remove the task from the task odict.
	 */
	irq_spinlock_lock(&tasks_lock, true);
	odict_remove(&task->ltasks);
	irq_spinlock_unlock(&tasks_lock, true);

	/*
	 * Perform architecture specific task destruction.
	 */
	task_destroy_arch(task);

	/*
	 * Drop our reference to the address space.
	 */
	as_release(task->as);

	slab_free(task_cache, task);
}

task_t *task_ref(task_t *task)
{
	if (kobj_ref(&task->kobj))
		return task;

	return NULL;
}

void task_put(task_t *task)
{
	if (task)
		kobj_put(&task->kobj);
}

/** Hold a reference to a task.
 *
 * Holding a reference to a task prevents destruction of that task.
 *
 * @param task Task to be held.
 *
 */
void task_hold(task_t *task)
{
	if (task)
		kobj_ref(&task->kobj);
}

/** Release a reference to a task.
 *
 * The last one to release a reference to a task destroys the task.
 *
 * @param task Task to be released.
 *
 */
void task_release(task_t *task)
{
	if (task)
		kobj_put(&task->kobj);
}

task_t *task_try_ref(task_t *task)
{
	if (task && kobj_try_ref(&task->kobj))
		return task;
	else
		return NULL;
}

sys_errno_t sys_task_get_id_2(sysarg_t task_handle, uspace_ptr_sysarg64_t uspace_taskid)
{
	task_id_t tid;

	if (!task_handle) {
		tid = TASK->taskid;
	} else {
		task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
		if (!task)
			return ENOENT;

		tid = task->taskid;
		task_put(task);
	}

	return (sys_errno_t) copy_to_uspace(uspace_taskid, &tid, sizeof(tid));
}

#ifdef __32_BITS__

/** Syscall for reading task ID from userspace (32 bits)
 *
 * @param uspace_taskid Pointer to user-space buffer
 *                      where to store current task ID.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_task_get_id(uspace_ptr_sysarg64_t uspace_taskid)
{
	/*
	 * No need to acquire lock on TASK because taskid remains constant for
	 * the lifespan of the task.
	 */
	return (sys_errno_t) copy_to_uspace(uspace_taskid, &TASK->taskid,
	    sizeof(TASK->taskid));
}

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

/** Syscall for reading task ID from userspace (64 bits)
 *
 * @return Current task ID.
 *
 */
sysarg_t sys_task_get_id(void)
{
	/*
	 * No need to acquire lock on TASK because taskid remains constant for
	 * the lifespan of the task.
	 */
	return TASK->taskid;
}

#endif  /* __64_BITS__ */

/** Syscall for setting the task name.
 *
 * The name simplifies identifying the task in the task list.
 *
 * @param name The new name for the task. (typically the same
 *             as the command used to execute it).
 *
 * @return 0 on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_task_set_name(const uspace_ptr_char uspace_name, size_t name_len)
{
	char namebuf[TASK_NAME_BUFLEN];

	/* Cap length of name and copy it from userspace. */
	if (name_len > TASK_NAME_BUFLEN - 1)
		name_len = TASK_NAME_BUFLEN - 1;

	errno_t rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != EOK)
		return (sys_errno_t) rc;

	namebuf[name_len] = '\0';

	/*
	 * As the task name is referenced also from the
	 * threads, lock the threads' lock for the course
	 * of the update.
	 */

	irq_spinlock_lock(&tasks_lock, true);
	irq_spinlock_lock(&TASK->lock, false);

	/* Set task name */
	str_cpy(TASK->name, TASK_NAME_BUFLEN, namebuf);

	irq_spinlock_unlock(&TASK->lock, false);
	irq_spinlock_unlock(&tasks_lock, true);

	return EOK;
}

/** Syscall to forcefully terminate a task
 *
 * @param uspace_taskid Pointer to task ID in user space.
 *
 * @return 0 on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_task_kill(uspace_ptr_task_id_t uspace_taskid)
{
	task_id_t taskid;
	errno_t rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(taskid));
	if (rc != EOK)
		return (sys_errno_t) rc;

	return (sys_errno_t) task_kill(taskid);
}

/** Find task structure corresponding to task ID.
 *
 * @param id Task ID.
 *
 * @return Task reference or NULL if there is no such task ID.
 *
 */
task_t *task_find_by_id(task_id_t id)
{
	task_t *task = NULL;

	irq_spinlock_lock(&tasks_lock, true);

	odlink_t *odlink = odict_find_eq(&tasks, &id, NULL);
	if (odlink != NULL) {
		/*
		 * The directory of tasks can't hold a reference, since that would
		 * prevent task from ever being destroyed. That means we have to
		 * check for the case where the task is already being destroyed, but
		 * not yet removed from the directory.
		 */
		task = task_try_ref(odict_get_instance(odlink, task_t, ltasks));
	}

	irq_spinlock_unlock(&tasks_lock, true);

	return task;
}

/** Get count of tasks.
 *
 * @return Number of tasks in the system
 */
size_t task_count(void)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

	return odict_count(&tasks);
}

/** Get first task (task with lowest ID).
 *
 * @return Pointer to first task or @c NULL if there are none.
 */
task_t *task_first(void)
{
	odlink_t *odlink;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

	odlink = odict_first(&tasks);
	if (odlink == NULL)
		return NULL;

	return odict_get_instance(odlink, task_t, ltasks);
}

/** Get next task (with higher task ID).
 *
 * @param cur Current task
 * @return Pointer to next task or @c NULL if there are no more tasks.
 */
task_t *task_next(task_t *cur)
{
	odlink_t *odlink;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

	odlink = odict_next(&cur->ltasks, &tasks);
	if (odlink == NULL)
		return NULL;

	return odict_get_instance(odlink, task_t, ltasks);
}

/** Get accounting data of given task.
 *
 * Note that task lock of 'task' must be already held and interrupts must be
 * already disabled.
 *
 * @param task    Pointer to the task.
 * @param ucycles Out pointer to sum of all user cycles.
 * @param kcycles Out pointer to sum of all kernel cycles.
 *
 */
void task_get_accounting(task_t *task, uint64_t *ucycles, uint64_t *kcycles)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&task->lock));

	/* Accumulated values of task */
	uint64_t uret = task->ucycles;
	uint64_t kret = task->kcycles;

	/* Current values of threads */
	list_foreach(task->threads, th_link, thread_t, thread) {
		/* Process only counted threads */
		if (!thread->uncounted) {
			if (thread == THREAD) {
				/* Update accounting of current thread */
				thread_update_accounting(false);
			}

			uret += atomic_time_read(&thread->ucycles);
			kret += atomic_time_read(&thread->kcycles);
		}
	}

	*ucycles = uret;
	*kcycles = kret;
}

static void task_kill_internal(task_t *task, int status)
{
	irq_spinlock_lock(&task->lock, true);

	/*
	 * Interrupt all threads.
	 */

	list_foreach(task->threads, th_link, thread_t, thread) {
		thread_interrupt(thread);
	}

	task->exit_status = status;

	irq_spinlock_unlock(&task->lock, true);

	//waitq_close(&task->join_wq);
}

/** Kill task.
 *
 * This function is idempotent.
 * It signals all the task's threads to bail it out.
 *
 * @param id ID of the task to be killed.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
errno_t task_kill(task_id_t id)
{
	if (id == 1)
		return EPERM;

	task_t *task = task_find_by_id(id);
	if (!task)
		return ENOENT;

	task_kill_internal(task, -1);
	task_release(task);
	return EOK;
}

/** Kill the currently running task.
 *
 * @param notify Send out fault notifications.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
void task_kill_self(bool notify, int status)
{
	/*
	 * User space can subscribe for FAULT events to take action
	 * whenever a task faults (to take a dump, run a debugger, etc.).
	 * The notification is always available, but unless udebug is enabled,
	 * that's all you get.
	 */
	if (notify) {
		/* Notify the subscriber that a fault occurred. */
		if (event_notify_3(EVENT_FAULT, false, LOWER32(TASK->taskid),
		    UPPER32(TASK->taskid), (sysarg_t) THREAD) == EOK) {
#ifdef CONFIG_UDEBUG
			/* Wait for a debugging session. */
			udebug_thread_fault();
#endif
		}
	}

	task_kill_internal(TASK, status);
	thread_exit();
}

/** Process syscall to terminate the current task.
 *
 * @param notify Send out fault notifications.
 *
 */
sys_errno_t sys_task_exit(sysarg_t notify, sysarg_t status)
{
	task_kill_self(notify, status);
	unreachable();
}

sys_errno_t sys_task_wait(sysarg_t task_handle, uspace_ptr_int uspace_status)
{
	// TODO
	return ENOSYS;
#if 0
	task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
	if (!task)
		return ENOENT;

	errno_t rc = waitq_sleep(&task->join_wq);
	int status = task->exit_status;
	task_put(task);

	if (rc == EOK && uspace_status != 0) {
		copy_to_uspace(uspace_status, &status, sizeof(status));
	}

	return rc;
#endif
}

static void task_print(task_t *task, bool additional)
{
	irq_spinlock_lock(&task->lock, false);

	uint64_t ucycles;
	uint64_t kcycles;
	char usuffix, ksuffix;
	task_get_accounting(task, &ucycles, &kcycles);
	order_suffix(ucycles, &ucycles, &usuffix);
	order_suffix(kcycles, &kcycles, &ksuffix);

#ifdef __32_BITS__
	if (additional)
		printf("%-8" PRIu64 " %9zu", task->taskid,
		    atomic_load(&task->lifecount));
	else
		printf("%-8" PRIu64 " %-14s %-5" PRIu32 " %10p %10p"
		    " %9" PRIu64 "%c %9" PRIu64 "%c\n", task->taskid,
		    task->name, task->container, task, task->as,
		    ucycles, usuffix, kcycles, ksuffix);
#endif

#ifdef __64_BITS__
	if (additional)
		printf("%-8" PRIu64 " %9" PRIu64 "%c %9" PRIu64 "%c "
		    "%9zu\n", task->taskid, ucycles, usuffix, kcycles,
		    ksuffix, atomic_load(&task->lifecount));
	else
		printf("%-8" PRIu64 " %-14s %-5" PRIu32 " %18p %18p\n",
		    task->taskid, task->name, task->container, task, task->as);
#endif

	irq_spinlock_unlock(&task->lock, false);
}

/** Print task list
 *
 * @param additional Print additional information.
 *
 */
void task_print_list(bool additional)
{
	/* Messing with task structures, avoid deadlock */
	irq_spinlock_lock(&tasks_lock, true);

#ifdef __32_BITS__
	if (additional)
		printf("[id    ] [threads] [calls] [callee\n");
	else
		printf("[id    ] [name        ] [ctn] [address ] [as      ]"
		    " [ucycles ] [kcycles ]\n");
#endif

#ifdef __64_BITS__
	if (additional)
		printf("[id    ] [ucycles ] [kcycles ] [threads] [calls]"
		    " [callee\n");
	else
		printf("[id    ] [name        ] [ctn] [address         ]"
		    " [as              ]\n");
#endif

	task_t *task;

	task = task_first();
	while (task != NULL) {
		task_print(task, additional);
		task = task_next(task);
	}

	irq_spinlock_unlock(&tasks_lock, true);
}

/** Get key function for the @c tasks ordered dictionary.
 *
 * @param odlink Link
 * @return Pointer to task ID cast as 'void *'
 */
static void *tasks_getkey(odlink_t *odlink)
{
	task_t *task = odict_get_instance(odlink, task_t, ltasks);
	return (void *) &task->taskid;
}

/** Key comparison function for the @c tasks ordered dictionary.
 *
 * @param a Pointer to thread A ID
 * @param b Pointer to thread B ID
 * @return -1, 0, 1 iff ID A is less than, equal to, greater than B
 */
static int tasks_cmp(void *a, void *b)
{
	task_id_t ida = *(task_id_t *)a;
	task_id_t idb = *(task_id_t *)b;

	if (ida < idb)
		return -1;
	else if (ida == idb)
		return 0;
	else
		return +1;
}

sysarg_t sys_task_create(uspace_ptr_const_char uspace_name, size_t name_len)
{
	char namebuf[TASK_NAME_BUFLEN];

	/* Cap length of name and copy it from userspace. */
	if (name_len > TASK_NAME_BUFLEN - 1)
		name_len = TASK_NAME_BUFLEN - 1;

	errno_t rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != EOK)
		return 0;

	namebuf[name_len] = '\0';

	as_t *child_as = as_create(0);
	if (!child_as)
		return 0;

	task_t *child = task_create(child_as, namebuf);
	if (!child) {
		as_release(child_as);
		return 0;
	}

	kobj_handle_t handle = kobj_table_insert(&TASK->kobj_table, &child->kobj);
	if (!handle)
		task_release(child);

	return handle;
}

sysarg_t sys_task_self(void)
{
	task_hold(TASK);
	kobj_handle_t handle = kobj_table_insert(&TASK->kobj_table, &TASK->kobj);
	if (!handle)
		task_release(TASK);
	return handle;
}

static errno_t task_mem_map(task_t *task, mem_t *mem, size_t offset, size_t size, uintptr_t *vaddr, int flags)
{
	if (!task)
		task = TASK;

	bool cow = (flags & AS_AREA_COW) != 0;
	if (cow) {
		flags ^= AS_AREA_COW;
		flags |= AS_AREA_WRITE;
	}

	mem_backend_t *backend = NULL;
	mem_backend_data_t backend_data = {};

	if (mem) {
		uint64_t allowed_size = mem_size(mem);
		int allowed_flags = mem_flags(mem) | AS_AREA_CACHEABLE | AS_AREA_GUARD | AS_AREA_LATE_RESERVE;

		if (cow) {
			allowed_flags |= AS_AREA_WRITE;
		}

		if (flags & ~allowed_flags) {
			printf("refused flags, allowed: 0%o, proposed: 0%o \n", allowed_flags, flags);
			return EINVAL;
		}

		if (allowed_size < offset || allowed_size - offset < size) {
			printf("refused size\n");
			return EINVAL;
		}

		backend = &mem_backend;
		backend_data = (mem_backend_data_t) {
			.mem = mem,
			.mem_offset = offset,
			.mem_cow = cow,
		};
	} else {
		backend = &anon_backend;
	}

	// task_t.as field is immutable after creation and has its own internal synchronization,
	// so digging into another task without further ado should be quite safe.
	as_area_t *area = as_area_create(task->as, flags, size,
	    AS_AREA_ATTR_NONE, backend, &backend_data, vaddr, 0);

	return area == NULL ? ENOMEM : EOK;
}

sys_errno_t sys_task_mem_map(sysarg_t task_handle, sysarg_t mem_handle, sysarg_t offset, sysarg_t size, uspace_ptr_uintptr_t uspace_vaddr, sysarg_t flags)
{
	printf("map: task_handle %d, mem_handle %d, offset %" PRIx64 ", size %" PRIx64 ", flags %d\n",
			(int)task_handle, (int)mem_handle, (uint64_t) offset, (uint64_t) size, (int)flags);

	task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
	if (task_handle && !task)
		return ENOENT;

	mem_t *mem = kobj_table_lookup(&TASK->kobj_table, mem_handle, KOBJ_CLASS_MEM);
	if (mem_handle && !mem) {
		task_put(task);
		return ENOENT;
	}

	uintptr_t vaddr;
	errno_t rc = copy_from_uspace(&vaddr, uspace_vaddr, sizeof(vaddr));
	if (rc == EOK) {
		printf("vaddr %" PRIxPTR"\n", vaddr);

		rc = task_mem_map(task, mem, offset, size, &vaddr, flags);
		printf("error: %s\n", str_error(rc));
		if (rc == EOK) {
			copy_to_uspace(uspace_vaddr, &vaddr, sizeof(vaddr));
			task_put(task);
			// mem reference is held by the as_area_t.
			return EOK;
		}
	}

	task_put(task);
	mem_put(mem);
	return rc;
}

sys_errno_t sys_task_mem_remap(sysarg_t task_handle, sysarg_t vaddr, sysarg_t size, sysarg_t new_flags)
{
	// TODO
	return EOK;
}

sys_errno_t sys_task_mem_unmap(sysarg_t task_handle, sysarg_t vaddr, sysarg_t size)
{
	// TODO
	return EOK;
}

errno_t task_mem_set(task_t *task, uintptr_t dst, int value, size_t size)
{
	errno_t rc;

	assert(task != NULL);

	// A quick hackjob. Dedicated userspace memset code would be preferrable.

	const int zero_len = 256;
	char zero[zero_len];
	memset(zero, value, zero_len);

	as_t *my_as = AS;
	as_t *their_as = task->as;

	if (my_as != their_as)
		as_switch(my_as, their_as);

	while (size > zero_len) {
		rc = copy_to_uspace(dst, zero, zero_len);
		if (rc != EOK)
			goto exit;

		dst += zero_len;
		size -= zero_len;
	}

	rc = copy_to_uspace(dst, zero, size);

exit:
	if (my_as != their_as)
		as_switch(their_as, my_as);

	return rc;
}

sys_errno_t sys_task_mem_set(sysarg_t task_handle, sysarg_t dst, sysarg_t value, sysarg_t size)
{
	task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
	errno_t rc = task_mem_set(task, dst, value, size);
	task_put(task);
	return (sys_errno_t) rc;
}

sysarg_t sys_mem_create(sysarg_t size, sysarg_t align, sysarg_t flags)
{
	mem_t *mem = mem_create(size, align, flags);
	if (!mem)
		return 0;

	kobj_handle_t handle = kobj_table_insert(&TASK->kobj_table, mem);
	if (!handle)
		mem_put(mem);

	return handle;
}

sys_errno_t sys_mem_change_flags(sysarg_t mem_handle, sysarg_t flags)
{
	mem_t *mem = kobj_table_lookup(&TASK->kobj_table, mem_handle, KOBJ_CLASS_MEM);
	if (!mem)
		return ENOENT;

	errno_t rc = mem_change_flags(mem, flags);
	mem_put(mem);
	return rc;
}

sys_errno_t sys_kobj_put(sysarg_t handle)
{
	kobj_t *kobj = kobj_table_remove(&TASK->kobj_table, handle);
	if (!kobj)
		return ENOENT;

	kobj_put(kobj);
	return EOK;
}

static errno_t task_thread_start(task_t *task, const char *name, uintptr_t entry, uintptr_t stack_base, uintptr_t stack_size)
{
	/*
	 * In case of failure, kernel_uarg will be deallocated in this function.
	 * In case of success, kernel_uarg will be freed in uinit().
	 */
	uspace_arg_t *kernel_uarg = malloc(sizeof(uspace_arg_t));
	if (!kernel_uarg)
		return ENOMEM;

	kernel_uarg->uspace_entry = entry;
	kernel_uarg->uspace_stack = stack_base;
	kernel_uarg->uspace_stack_size = stack_size;
	kernel_uarg->uspace_thread_function = 0;
	kernel_uarg->uspace_thread_arg = 0;
	kernel_uarg->uspace_uarg = 0;

	thread_t *thread = thread_create(uinit, kernel_uarg, task, THREAD_FLAG_USPACE| THREAD_FLAG_NOATTACH, name);
	if (!thread) {
		free(kernel_uarg);
		return ENOMEM;
	}

#ifdef CONFIG_UDEBUG
	/*
	 * Generate udebug THREAD_B event and attach the thread.
	 * This must be done atomically (with the debug locks held),
	 * otherwise we would either miss some thread or receive
	 * THREAD_B events for threads that already existed
	 * and could be detected with THREAD_READ before.
	 */
	udebug_thread_b_event_attach(thread, task);
#else
	thread_attach(thread, task);
#endif

	thread_ready(thread);
	return EOK;
}

sys_errno_t sys_task_thread_start(sysarg_t task_handle, uspace_ptr_char uspace_name, sysarg_t name_len, sysarg_t pc, sysarg_t stack_base, sysarg_t stack_size)
{
	if (name_len > THREAD_NAME_BUFLEN - 1)
		name_len = THREAD_NAME_BUFLEN - 1;

	char namebuf[THREAD_NAME_BUFLEN];
	errno_t rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != EOK)
		return (sys_errno_t) rc;

	namebuf[name_len] = '\0';

	task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
	if (!task)
		return (sys_errno_t) ENOENT;

	rc = task_thread_start(task, namebuf, pc, stack_base, stack_size);
	task_put(task);

	return (sys_errno_t) rc;
}

sys_errno_t sys_task_connect(sysarg_t task_handle, uspace_ptr_cap_phone_handle_t uspace_phone)
{
	task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
	if (!task)
		return ENOENT;

	cap_phone_handle_t phandle;
	kobject_t *pobj;
	errno_t rc = phone_alloc(TASK, false, &phandle, &pobj);
	if (rc != EOK) {
		task_put(task);
		return rc;
	}

	if (ipc_phone_connect(pobj->phone, &task->answerbox)) {
		task_put(task);
		cap_publish(TASK, phandle, pobj);
		copy_to_uspace(uspace_phone, &phandle, sizeof(phandle));
		return EOK;
	} else {
		task_put(task);
		return ENOENT;
	}
}

errno_t task_mem_read(task_t *task, uspace_addr_t addr, void *dst, size_t size)
{
	assert(task != NULL);

	as_t *my_as = AS;
	as_t *their_as = task->as;

	if (my_as != their_as)
		as_switch(my_as, their_as);

	errno_t rc = copy_from_uspace(dst, addr, size);

	if (my_as != their_as)
		as_switch(their_as, my_as);

	return rc;
}

errno_t task_mem_write(task_t *task, uspace_addr_t addr, const void *src, size_t size)
{
	assert(task != NULL);

	as_t *my_as = AS;
	as_t *their_as = task->as;

	if (my_as != their_as)
		as_switch(my_as, their_as);

	errno_t rc = copy_to_uspace(addr, src, size);

	if (my_as != their_as)
		as_switch(their_as, my_as);

	return rc;
}

// Not exactly the most efficient way to transfer data between tasks, but works in a pinch.
sys_errno_t sys_task_mem_write(sysarg_t task_handle, uspace_addr_t dst, uspace_addr_t src, size_t size)
{
	task_t *task = kobj_table_lookup(&TASK->kobj_table, task_handle, KOBJ_CLASS_TASK);
	if (!task)
		return ENOENT;

	const size_t MAX_WRITE_SIZE = 1024;
	char buffer[MAX_WRITE_SIZE];

	errno_t rc;

	while (size > MAX_WRITE_SIZE) {
		rc = copy_from_uspace(buffer, src, MAX_WRITE_SIZE);
		if (rc != EOK)
			goto exit;

		rc = task_mem_write(task, dst, buffer, MAX_WRITE_SIZE);
		if (rc != EOK)
			goto exit;

		dst += MAX_WRITE_SIZE;
		src += MAX_WRITE_SIZE;
		size -= MAX_WRITE_SIZE;
	}

	rc = copy_from_uspace(buffer, src, size);
	if (rc != EOK)
		goto exit;

	rc = task_mem_write(task, dst, buffer, size);
	if (rc != EOK)
		goto exit;

exit:
	task_put(task);
	return rc;
}

/** @}
 */
