_name = 'ar9271'
_deps = [ libc, libusbdev , libusb , libdrv , libnic , libieee80211 , libcrypto ]
_extra_cflags = [ '-I.', ]
_includes = include_directories('.')
_src = files(
	'ath_usb.c',
	'hw.c',
	'wmi.c',
	'htc.c',
	'ar9271.c',
)

ar9271 = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
