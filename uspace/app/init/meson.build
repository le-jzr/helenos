_name = 'init'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'init.c',
)

init = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
