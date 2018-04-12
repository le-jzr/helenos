_name = 'modplay'
_deps = [ libc, libtrackmod , libhound , libpcm ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'modplay.c',
)

modplay = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
