_name = 'ethip'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'arp.c',
	'atrans.c',
	'ethip.c',
	'ethip_nic.c',
	'pdu.c',
)

ethip = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
