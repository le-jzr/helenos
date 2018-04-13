_libname = 'usbhost'
_deps = [ libc, libusb , libdrv ]
_extra_cflags = [ ]
_exports = include_directories('include')
_includes = [ _exports, include_directories('include/usb/host') ]
_src = files(
	'src/ddf_helpers.c',
	'src/endpoint.c',
	'src/hcd.c',
	'src/bus.c',
	'src/usb2_bus.c',
	'src/bandwidth.c',
	'src/utility.c',
	'src/usb_transfer_batch.c',
)

libusbhost = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
