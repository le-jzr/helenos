_name = 'inetsrv'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'addrobj.c',
	'icmp.c',
	'icmpv6.c',
	'inetsrv.c',
	'inet_link.c',
	'inetcfg.c',
	'inetping.c',
	'ndp.c',
	'ntrans.c',
	'pdu.c',
	'reass.c',
	'sroute.c',
)

inetsrv = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
