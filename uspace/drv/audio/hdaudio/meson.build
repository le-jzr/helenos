_name = 'hdaudio'
_deps = [ libc, libdrv , libpcm ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'codec.c',
	'regif.c',
	'hdactl.c',
	'hdaudio.c',
	'pcm_iface.c',
	'stream.c',
)

hdaudio = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
