
#include <abi/syscall.h>
#include <ipc_b.h>
#include <libc.h>
#include <panic.h>
#include <protocol/root.h>
#include <stdatomic.h>
#include <str.h>

static _Atomic(ipc_endpoint_t *) _root_handle = 0;

static ipc_endpoint_t *_root_ep()
{
	auto handle = atomic_load_explicit(&_root_handle, memory_order_relaxed);

	if (handle == NULL) {
		handle = (ipc_endpoint_t *) __SYSCALL0(SYS_IPCB_NS_GET);
		auto old = atomic_exchange_explicit(&_root_handle, handle, memory_order_relaxed);
		if (old != NULL)
			ipc_endpoint_put(old);
	}

	return handle;
}

void ipc_root_serve(const ipc_root_server_ops_t *ops)
{
    panic("unimplemented");
}

ipc_root_retval_t ipc_root_send(const char *name, const ipc_message_t *args)
{
    size_t name_sz = str_size(name);

    auto b = ipc_blob_create(name, name_sz);

    ipc_message_t msg = *args;
    ipc_message_prepend(&msg, (sysarg_t) name_sz);
    ipc_message_prepend(&msg, b);

    ipcb_send(_root_ep(), &msg);
    panic("uimplemented");
}

#if 0

enum {
    IPC_ROOT_SET = 0,
};

cap_handle_t ipc_root_get(const char *name)
{
	size_t name_sz = str_size(name);

	ipc_message_t call = {};
	ipc_set_arg(&call, 1, IPC_ROOT_GET);

	ipc_message_t reply = {};
	ipc_call_long(_root_ep(), &reply, &call, name, name_sz);

	if (reply.flags != ipc_message_flags_2(0, IPC_ARG_TYPE_NONE, IPC_ARG_TYPE_OBJECT)) {
		ipc_message_drop(&reply);
		return CAP_NIL;
	}

	return ipc_get_cap(&reply, 1);
}

bool ipc_root_set(const char *name, cap_handle_t handle)
{
	size_t name_sz = str_size(name);

	ipc_message_t call = {};
	ipc_set_arg(&call, 1, IPC_ROOT_SET);
	ipc_set_arg(&call, 2, handle);

	ipc_message_t reply = {};
	ipc_call_long(_root_ep(), &reply, &call, name, name_sz);

	if (reply.flags != ipc_message_flags_2(0, IPC_ARG_TYPE_NONE, IPC_ARG_TYPE_VAL)) {
		ipc_message_drop(&reply);
		return false;
	}

	return ipc_get_val(&reply, 1);
}

#endif
