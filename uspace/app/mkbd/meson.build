_name = 'mkbd'
_deps = [ libc, libusb , libusbdev , libusbhid , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
)

mkbd = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
