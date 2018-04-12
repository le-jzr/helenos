_name = 'ddisk'
_deps = [ libc, libdrv , libscsi ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ddisk.c',
)

ddisk = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
