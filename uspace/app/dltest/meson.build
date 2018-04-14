_name = 'dltest'
_deps = [ libc, libdltest ]
_src = files(
	'dltest.c',
)

dltest = executable(_name, _src,
	dependencies: _deps,
	c_args: [ '-DDLTEST_LINKED' ],
	link_args: [ '-Wl,-Bdynamic' ],
)
