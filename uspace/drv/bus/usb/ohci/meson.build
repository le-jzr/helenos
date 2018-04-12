_name = 'ohci'
_deps = [ libc, libusbhost , libusbvirt , libusb , libdrv , libusbdev ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'endpoint_list.c',
	'hc.c',
	'main.c',
	'ohci_batch.c',
	'ohci_bus.c',
	'ohci_rh.c',
	'hw_struct/endpoint_descriptor.c',
	'hw_struct/transfer_descriptor.c',
)

ohci = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
