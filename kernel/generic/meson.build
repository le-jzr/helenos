GENERIC_SOURCES = \
	generic/src/adt/avl.c \
	generic/src/adt/bitmap.c \
	generic/src/adt/btree.c \
	generic/src/adt/cht.c \
	generic/src/adt/hash_table.c \
	generic/src/adt/list.c \
	generic/src/console/chardev.c \
	generic/src/console/console.c \
	generic/src/console/prompt.c \
	generic/src/cpu/cpu.c \
	generic/src/cpu/cpu_mask.c \
	generic/src/ddi/ddi.c \
	generic/src/ddi/irq.c \
	generic/src/debug/symtab.c \
	generic/src/debug/stacktrace.c \
	generic/src/debug/panic.c \
	generic/src/debug/debug.c \
	generic/src/interrupt/interrupt.c \
	generic/src/log/log.c \
	generic/src/main/main.c \
	generic/src/main/kinit.c \
	generic/src/main/uinit.c \
	generic/src/main/version.c \
	generic/src/main/shutdown.c \
	generic/src/proc/program.c \
	generic/src/proc/scheduler.c \
	generic/src/proc/thread.c \
	generic/src/proc/task.c \
	generic/src/proc/the.c \
	generic/src/syscall/syscall.c \
	generic/src/syscall/copy.c \
	generic/src/mm/km.c \
	generic/src/mm/reserve.c \
	generic/src/mm/frame.c \
	generic/src/mm/page.c \
	generic/src/mm/tlb.c \
	generic/src/mm/as.c \
	generic/src/mm/backend_anon.c \
	generic/src/mm/backend_elf.c \
	generic/src/mm/backend_phys.c \
	generic/src/mm/backend_user.c \
	generic/src/mm/slab.c \
	generic/src/lib/halt.c \
	generic/src/lib/mem.c \
	generic/src/lib/memfnc.c \
	generic/src/lib/gsort.c \
	generic/src/lib/str.c \
	generic/src/lib/str_error.c \
	generic/src/lib/elf.c \
	generic/src/lib/ra.c \
	generic/src/lib/rd.c \
	generic/src/printf/printf_core.c \
	generic/src/printf/printf.c \
	generic/src/printf/snprintf.c \
	generic/src/printf/vprintf.c \
	generic/src/printf/vsnprintf.c \
	generic/src/time/clock.c \
	generic/src/time/timeout.c \
	generic/src/time/delay.c \
	generic/src/preempt/preemption.c \
	generic/src/synch/spinlock.c \
	generic/src/synch/condvar.c \
	generic/src/synch/mutex.c \
	generic/src/synch/semaphore.c \
	generic/src/synch/smc.c \
	generic/src/synch/smp_memory_barrier.c \
	generic/src/synch/waitq.c \
	generic/src/synch/futex.c \
	generic/src/synch/workqueue.c \
	generic/src/synch/rcu.c \
	generic/src/smp/ipi.c \
	generic/src/smp/smp.c \
	generic/src/smp/smp_call.c \
	generic/src/ipc/ipc.c \
	generic/src/ipc/sysipc.c \
	generic/src/ipc/sysipc_ops.c \
	generic/src/ipc/ops/conctmeto.c \
	generic/src/ipc/ops/concttome.c \
	generic/src/ipc/ops/dataread.c \
	generic/src/ipc/ops/datawrite.c \
	generic/src/ipc/ops/debug.c \
	generic/src/ipc/ops/pagein.c \
	generic/src/ipc/ops/sharein.c \
	generic/src/ipc/ops/shareout.c \
	generic/src/ipc/ops/stchngath.c \
	generic/src/ipc/ipcrsc.c \
	generic/src/ipc/irq.c \
	generic/src/ipc/event.c \
	generic/src/cap/cap.c \
	generic/src/security/perm.c \
	generic/src/sysinfo/sysinfo.c \
	generic/src/sysinfo/stats.c

## Kernel console support
#

ifeq ($(CONFIG_KCONSOLE),y)
GENERIC_SOURCES += \
	generic/src/console/kconsole.c \
	generic/src/console/cmd.c
endif

## Udebug interface sources
#

ifeq ($(CONFIG_UDEBUG),y)
GENERIC_SOURCES += \
	generic/src/ipc/kbox.c \
	generic/src/udebug/udebug.c \
	generic/src/udebug/udebug_ops.c \
	generic/src/udebug/udebug_ipc.c
endif

## Test sources
#

ifeq ($(CONFIG_TEST),y)
	CFLAGS += -Itest/
	GENERIC_SOURCES += \
		test/test.c \
		test/atomic/atomic1.c \
		test/btree/btree1.c \
		test/cht/cht1.c \
		test/avltree/avltree1.c \
		test/fault/fault1.c \
		test/mm/falloc1.c \
		test/mm/falloc2.c \
		test/mm/mapping1.c \
		test/mm/slab1.c \
		test/mm/slab2.c \
		test/synch/semaphore1.c \
		test/synch/semaphore2.c \
		test/synch/workqueue2.c \
		test/synch/workqueue3.c \
		test/synch/rcu1.c \
		test/print/print1.c \
		test/print/print2.c \
		test/print/print3.c \
		test/print/print4.c \
		test/print/print5.c \
		test/thread/thread1.c \
		test/smpcall/smpcall1.c

	ifeq ($(KARCH),mips32)
		GENERIC_SOURCES += test/debug/mips1.c
	else
		GENERIC_SOURCES += test/debug/mips1_skip.c
	endif

	ifeq ($(KARCH),ia64)
		GENERIC_SOURCES += test/mm/purge1.c
	else
		GENERIC_SOURCES += test/mm/purge1_skip.c
	endif

endif