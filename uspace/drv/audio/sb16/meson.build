_name = 'sb16'
_deps = [ libc, libdrv , libpcm ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'dsp.c',
	'main.c',
	'mixer.c',
	'mixer_iface.c',
	'pcm_iface.c',
	'sb16.c',
)

sb16 = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
