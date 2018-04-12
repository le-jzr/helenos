_name = 'barber'
_deps = [ libc, libgui, libdraw, libcompress, libsoftrend, libmath ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'barber.c',
	'images.s',
	'images_desc.c',
)

barber = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
