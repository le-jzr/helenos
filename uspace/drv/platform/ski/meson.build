_name = 'ski'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ski.c',
)

ski = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
