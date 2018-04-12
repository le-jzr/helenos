_name = 'wavplay'
_deps = [ libc, libdrv , libhound , libpcm ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'dplay.c',
	'drec.c',
	'main.c',
	'wave.c',
)

wavplay = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
