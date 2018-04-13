_name = 'trace'
_deps = [ libc ]
_extra_cflags = [ ]
_src = files(
	'trace.c',
	'syscalls.c',
	'ipcp.c',
	'ipc_desc.c',
	'proto.c',
)

trace = executable(_name, _src,
	dependencies: _deps,
	c_args: _extra_cflags,
	implicit_include_directories: false,
)
