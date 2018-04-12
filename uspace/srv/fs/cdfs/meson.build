_name = 'cdfs'
_deps = [ libc, libblock , libfs , libscsi ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'cdfs.c',
	'cdfs_ops.c',
)

cdfs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
