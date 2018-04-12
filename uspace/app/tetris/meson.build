_name = 'tetris'
_deps = [ libc ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'shapes.c',
	'scores.c',
	'tetris.c',
	'screen.c',
)

tetris = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
