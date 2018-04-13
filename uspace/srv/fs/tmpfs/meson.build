_name = 'tmpfs'
_deps = [ libc, libblock , libfs ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'tmpfs.c',
	'tmpfs_ops.c',
	'tmpfs_dump.c',
)

tmpfs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
