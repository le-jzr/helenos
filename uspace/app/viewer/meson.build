_name = 'viewer'
_deps = [ libc, libgui , libdraw , libsoftrend , libcompress , libmath ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'viewer.c',
)

viewer = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
