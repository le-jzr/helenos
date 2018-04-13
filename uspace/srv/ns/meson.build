_name = 'ns'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ns.c',
	'service.c',
	'clonable.c',
	'task.c',
)

ns = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
