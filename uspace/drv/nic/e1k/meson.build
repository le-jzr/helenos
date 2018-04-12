_name = 'e1k'
_deps = [ libc, libdrv , libnic ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'e1k.c',
)

e1k = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
