_name = 'vuh'
_deps = [ libc, libusb , libusbdev , libusbhid , libusbvirt , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
	'device.c',
	'ifaces.c',
	'life.c',
	'stdreq.c',
	'hids/bootkbd.c',
	'hids/logitech_wireless.c',
)

vuh = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
