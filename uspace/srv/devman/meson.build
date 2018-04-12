_name = 'devman'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
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
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
