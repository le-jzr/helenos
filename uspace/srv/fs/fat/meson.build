_name = 'fat'
_deps = [ libc, libblock , libfs ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'fat.c',
	'fat_ops.c',
	'fat_idx.c',
	'fat_dentry.c',
	'fat_directory.c',
	'fat_fat.c',
)

fat = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
