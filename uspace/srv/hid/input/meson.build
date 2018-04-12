_name = 'input'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'layout/cz.c',
	'layout/us_qwerty.c',
	'layout/us_dvorak.c',
	'layout/ar.c',
	'port/chardev.c',
	'proto/mousedev.c',
	'ctl/kbdev.c',
	'ctl/stty.c',
	'ctl/sun.c',
	'gsp.c',
	'input.c',
	'layout.c',
	'stroke.c',
)

input = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
