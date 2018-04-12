_name = 'usbhub'
_deps = [ libc, libusbdev , libusb , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
	'usbhub.c',
	'port.c',
)

usbhub = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
