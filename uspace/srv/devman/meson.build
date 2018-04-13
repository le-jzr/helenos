_name = 'devman'
_deps = [ libc ]
_extra_cflags = [ ]
_src = files(
	'client_conn.c',
	'dev.c',
	'devtree.c',
	'drv_conn.c',
	'driver.c',
	'loc.c',
	'fun.c',
	'main.c',
	'match.c',
	'util.c',
)

devman = executable(_name, _src,
	implicit_include_directories: false,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
