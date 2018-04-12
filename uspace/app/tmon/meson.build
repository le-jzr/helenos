_name = 'tmon'
_deps = [ libc, libdrv , libusb ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
	'list.c',
	'tf.c',
	'tests.c',
	'resolve.c',
)

tmon = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
