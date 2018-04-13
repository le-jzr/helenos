_name = 'logger'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ctl.c',
	'initlvl.c',
	'level.c',
	'logs.c',
	'main.c',
	'writer.c',
)

logger = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
	link_args: [ '-static' ],
)
