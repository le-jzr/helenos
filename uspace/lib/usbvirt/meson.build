_libname = 'usbvirt'
_deps = [ libc, libdrv , libusb , libusbdev ]
_extra_cflags = [ '-Iinclude/usbvirt', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/ctrltransfer.c',
	'src/device.c',
	'src/ipc_dev.c',
	'src/ipc_hc.c',
	'src/stdreq.c',
	'src/transfer.c',
	'src/virthub_base.c',
	'src/virthub_descriptors.c',
)

libusbvirt = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
