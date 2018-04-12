_name = 'loader'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'main.c',
)

loader = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
