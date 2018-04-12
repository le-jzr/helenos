_name = 'trace'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'trace.c',
	'syscalls.c',
	'ipcp.c',
	'ipc_desc.c',
	'proto.c',
)

trace = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
