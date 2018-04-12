_name = 'uhci'
_deps = [ libc, libusbhost , libusbvirt , libusb , libdrv , libusbdev ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'hc.c',
	'main.c',
	'transfer_list.c',
	'uhci_batch.c',
	'uhci_rh.c',
	'hw_struct/transfer_descriptor.c',
)

uhci = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
