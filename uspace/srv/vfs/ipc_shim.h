#include <abi/ipc_b.h>
#include <offset.h>

typedef struct vfs_instance vfs_instance_t;
typedef struct vfs_instance_impl vfs_instance_impl_t;


// remove
typedef ipc_message_t ipcb_message_t;

static inline uintptr_t ipcb_get_val_1(const ipc_message_t *m)
{
	return ipc_get_val(m, 1);
}
