_libname = 'usbhid'
_deps = [ libc, libusb , libusbdev , libdrv ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/hiddescriptor.c',
	'src/hidparser.c',
	'src/hidpath.c',
	'src/hidreport.c',
	'src/consumer.c',
	'src/hidreq.c',
)

libusbhid = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
