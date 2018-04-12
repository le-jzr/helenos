_name = 'test1'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'test1.c',
)

test1 = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
