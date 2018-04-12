_name = 'dnsrsrv'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'dns_msg.c',
	'dnsrsrv.c',
	'query.c',
	'transport.c',
)

dnsrsrv = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
