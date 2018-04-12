_name = 'vfs'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'vfs.c',
	'vfs_node.c',
	'vfs_file.c',
	'vfs_ops.c',
	'vfs_lookup.c',
	'vfs_register.c',
	'vfs_ipc.c',
	'vfs_pager.c',
)

vfs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
