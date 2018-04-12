_name = 'locfs'
_deps = [ libc, libfs ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'locfs.c',
	'locfs_ops.c',
)

locfs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
