
#include <ipc/root.h>

#include <ipc_b.h>
#include <stdatomic.h>

static atomic(cap_handle_t) _root_handle = 0;

static ipc_endpoint_t *_root_ep()
{
	auto handle = atomic_load_explicit(&_root_handle, atomic_relaxed);

	if (handle == CAP_NIL) {
		handle = __syscall0(SYS_ROOT_GET);
		auto old = atomic_exchange_explicit(&_root_handle, handle, atomic_relaxed);
		if (old != CAP_NIL)
			cap_drop(old);
	}

	return handle;
}

cap_handle_t ipc_root_get(const char *name)
{
	size_t name_sz = str_size(name);

	ipc_message_t call = {};
	ipc_set_arg(&call, 1, IPC_ROOT_GET);

	ipc_message_t reply = {};
	ipc_call_long(_root_ep(), &reply, &call, name, name_sz);

	if (reply.flags != ipc_message_flags_2(0, IPC_ARG_TYPE_NONE, IPC_ARG_TYPE_CAP)) {
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
