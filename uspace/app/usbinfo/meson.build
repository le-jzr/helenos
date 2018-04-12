_name = 'usbinfo'
_deps = [ libc, libusbhid , libusbdev , libusb , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'desctree.c',
	'dump.c',
	'hid.c',
	'info.c',
	'list.c',
	'main.c',
)

usbinfo = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
