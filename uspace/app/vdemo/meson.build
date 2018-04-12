_name = 'vdemo'
_deps = [ libc, libgui , libdraw , libsoftrend , libcompress , libmath ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'vdemo.c',
)

vdemo = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
