_name = 'sun4v-con'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
	'sun4v-con.c',
)

sun4v_con = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
