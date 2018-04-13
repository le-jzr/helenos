_name = 'viewer'
_deps = [ libgui ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'viewer.c',
)

viewer = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
