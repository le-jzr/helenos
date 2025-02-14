
typedef enum ipc_node_method {
	ipc_node_copy,
	ipc_node_fsprobe,
	ipc_node_fstypes,
	ipc_node_mount,
	ipc_node_move,
	ipc_node_open,
	ipc_node_stat,
	ipc_node_statfs,
	ipc_node_sync,
	ipc_node_unlink,
	ipc_node_unmount,
	ipc_node_walk,
} ipc_node_method_t;

typedef enum ipc_file_method {
	ipc_file_read,
	ipc_file_reopen,
	ipc_file_resize,
	ipc_file_stat,
	ipc_file_sync,
	ipc_file_write,
} ipc_file_method_t;


