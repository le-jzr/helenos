_name = 'bithenge'
_deps = [ libc, libbithenge, libblock ]
_extra_cflags = [ '-D__HELENOS__', ]
_includes = include_directories('.')
_src = files(
	'test.c',
)

bithenge = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
