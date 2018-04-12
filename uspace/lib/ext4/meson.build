_libname = 'ext4'
_deps = [ libc, libblock , libcrypto , libfs ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/balloc.c',
	'src/bitmap.c',
	'src/block_group.c',
	'src/directory.c',
	'src/directory_index.c',
	'src/extent.c',
	'src/filesystem.c',
	'src/hash.c',
	'src/ialloc.c',
	'src/inode.c',
	'src/ops.c',
	'src/superblock.c',
)

libext4 = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
