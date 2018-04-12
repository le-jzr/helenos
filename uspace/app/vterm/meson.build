_name = 'vterm'
_deps = [ libc, libgui , libgraph , libdraw , libsoftrend , libcompress , libmath ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'vterm.c',
)

vterm = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
