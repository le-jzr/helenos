_name = 'udp'
_deps = [ libc, libnettl ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'assoc.c',
	'msg.c',
	'pdu.c',
	'service.c',
	'udp.c',
	'udp_inet.c',
)

udp = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
