_name = 'locsrv'
_deps = [ libc ]
_extra_cflags = [ ]
_src = files(
	'category.c',
	'locsrv.c',
)

locsrv = executable(_name, _src,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
