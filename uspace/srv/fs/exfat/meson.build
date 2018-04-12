_name = 'exfat'
_deps = [ libc, libblock , libfs ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'exfat.c',
	'exfat_fat.c',
	'exfat_bitmap.c',
	'exfat_ops.c',
	'exfat_idx.c',
	'exfat_dentry.c',
	'exfat_directory.c',
)

exfat = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
