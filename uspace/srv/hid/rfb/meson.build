_name = 'rfb'
_deps = [ libc, libgraph ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
	'rfb.c',
)

rfb = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
