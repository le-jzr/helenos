_name = 'ne2k'
_deps = [ libc, libdrv , libnic ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'dp8390.c',
	'ne2k.c',
)

ne2k = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
