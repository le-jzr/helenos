_name = 'usbdiag'
_deps = [ libc, libusbdev , libusb , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'device.c',
	'tests.c',
	'main.c',
)

usbdiag = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
