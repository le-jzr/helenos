_name = 'mfs'
_deps = [ libc, libblock , libfs , libminix ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'mfs.c',
	'mfs_ops.c',
	'mfs_inode.c',
	'mfs_rw.c',
	'mfs_dentry.c',
	'mfs_balloc.c',
	'mfs_utils.c',
)

mfs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
