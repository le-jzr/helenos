_name = 'usbmast'
_deps = [ libc, libusbdev , libusb , libdrv , libscsi ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'bo_trans.c',
	'cmdw.c',
	'main.c',
	'scsi_ms.c',
)

usbmast = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
