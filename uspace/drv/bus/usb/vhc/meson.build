_name = 'vhc'
_deps = [ libc, libusbdev , libusbhost , libusbvirt , libusb , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'hub/virthub.c',
	'hub/hub.c',
	'hub/virthubops.c',
	'conndev.c',
	'devconn.c',
	'main.c',
	'transfer.c',
)

vhc = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
