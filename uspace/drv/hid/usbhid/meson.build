_name = 'usbhid'
_deps = [ libc, libusbhid , libusbdev , libusb , libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
	'usbhid.c',
	'subdrivers.c',
	'generic/hiddev.c',
	'kbd/conv.c',
	'kbd/kbddev.c',
	'kbd/kbdrepeat.c',
	'mouse/mousedev.c',
	'multimedia/multimedia.c',
	'multimedia/keymap.c',
	'blink1/blink1.c',
)

usbhid = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
