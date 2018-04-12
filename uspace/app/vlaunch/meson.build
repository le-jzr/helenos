_name = 'vlaunch'
_deps = [ libc, libgui , libdraw , libsoftrend , libcompress , libmath ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'vlaunch.c',
	'images.s',
	'images_desc.c',
)

vlaunch = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
