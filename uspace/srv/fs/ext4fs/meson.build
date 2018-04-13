_name = 'ext4fs'
_deps = [ libc, libext4 , libblock , libfs , libcrypto ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ext4fs.c',
)

ext4fs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
