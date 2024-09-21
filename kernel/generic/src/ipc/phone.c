
#include <ipc/ipc.h>

struct phone {
	mutex_t lock;
	link_t link;
	struct task *caller;
	struct answerbox *callee;
	/* A call prepared for hangup ahead of time, so that it cannot fail. */
	struct call *hangup_call;
	ipc_phone_state_t state;
	atomic_size_t active_calls;
	/** User-defined label */
	sysarg_t label;
	kobject_t *kobject;
};

void ipc_phone_set_label(phone_t *phone, sysarg_t label)
{
	phone->label = label;
}

phone_t *ipc_phone_ref(phone_t *phone)
{
	kobject_add_ref(phone->kobject);
	return phone;
}

void ipc_phone_put(phone_t *phone)
{
	kobject_put(phone->kobject);
}

void ipc_phone_print_state(phone_t *phone, int handle)
{
	mutex_lock(&phone->lock);
	if (phone->state != IPC_PHONE_FREE) {
		printf("%-11d %7" PRIun " ", (int) cap_handle_raw(cap->handle),
		    atomic_load(&phone->active_calls));

		switch (phone->state) {
		case IPC_PHONE_CONNECTING:
			printf("connecting");
			break;
		case IPC_PHONE_CONNECTED:
			printf("connected to %" PRIu64 " (%s)",
			    phone->callee->task->taskid,
			    phone->callee->task->name);
			break;
		case IPC_PHONE_SLAMMED:
			printf("slammed by %p", phone->callee);
			break;
		case IPC_PHONE_HUNGUP:
			printf("hung up to %p", phone->callee);
			break;
		default:
			break;
		}

		printf("\n");
	}
	mutex_unlock(&phone->lock);
}

void ipc_phone_add_call(phone_t *phone)
{
	atomic_inc(&phone->active_calls);
}

void ipc_phone_remove_call(phone_t *phone)
{
	atomic_dec(&phone->active_calls);
}

bool ipc_phone_within_call_limit(phone_t *phone)
{
	return atomic_load(&phone->active_calls) < IPC_MAX_ASYNC_CALLS;
}

void ipc_phone_unlink_from_box(phone_t *phone, answerbox_t *box)
{

}

void ipc_phone_slam(phone_t *phone)
{

}

/** Connect a phone to an answerbox.
 *
 * This function must be passed a reference to phone->kobject.
 *
 * @param phone  Initialized phone structure.
 * @param box    Initialized answerbox structure.
 * @return       True if the phone was connected, false otherwise.
 */
bool ipc_phone_connect(phone_t *phone, answerbox_t *box)
{
	bool connected;

	mutex_lock(&phone->lock);
	irq_spinlock_lock(&box->lock, true);

	connected = box->active && (phone->state == IPC_PHONE_CONNECTING);
	if (connected) {
		phone->state = IPC_PHONE_CONNECTED;
		phone->callee = box;
		/* Pass phone->kobject reference to box->connected_phones */
		list_append(&phone->link, &box->connected_phones);
	}

	irq_spinlock_unlock(&box->lock, true);
	mutex_unlock(&phone->lock);

	if (!connected) {
		/* We still have phone->kobject's reference; drop it */
		kobject_put(phone->kobject);
	}

	return connected;
}

/** Initialize a phone structure.
 *
 * @param phone Phone structure to be initialized.
 * @param caller Owning task.
 *
 */
void ipc_phone_init(phone_t *phone, task_t *caller)
{
	mutex_initialize(&phone->lock, MUTEX_PASSIVE);
	phone->caller = caller;
	phone->callee = NULL;
	phone->state = IPC_PHONE_FREE;
	atomic_store(&phone->active_calls, 0);
	phone->label = 0;
	phone->kobject = NULL;
}
