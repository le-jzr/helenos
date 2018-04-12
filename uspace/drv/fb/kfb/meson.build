_name = 'kfb'
_deps = [ libc, libdrv , libgraph , libsoftrend ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'port.c',
	'kfb.c',
)

kfb = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
