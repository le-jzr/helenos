_name = 'xhci'
_deps = [ libc, libusbhost , libusbvirt , libusb , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'bus.c',
	'commands.c',
	'debug.c',
	'device.c',
	'endpoint.c',
	'hc.c',
	'isoch.c',
	'main.c',
	'rh.c',
	'scratchpad.c',
	'streams.c',
	'transfers.c',
	'trb_ring.c',
)

xhci = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)

_test_src = files(
	'test/reg-ops.c',
)

test_xhci = executable('test_xhci', _test_src,
	include_directories: _includes,
	dependencies: [ _deps, libpcut ],
)
