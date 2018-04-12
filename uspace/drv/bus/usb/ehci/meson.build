_name = 'ehci'
_deps = [ libc, libusbhost , libusbvirt , libusb , libdrv , libusbdev ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ehci_batch.c',
	'ehci_bus.c',
	'ehci_rh.c',
	'endpoint_list.c',
	'hc.c',
	'hw_struct/queue_head.c',
	'hw_struct/transfer_descriptor.c',
	'main.c',
	'res.c',
)

ehci = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
