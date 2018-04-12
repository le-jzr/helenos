_name = 'amdm37x_dispc'
_deps = [ libc, libdrv , libgraph , libsoftrend ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'amdm37x_dispc.c',
	'main.c',
)

amdm37x_dispc = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
