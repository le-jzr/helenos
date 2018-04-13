_name = 'hound'
_deps = [ libc, libdrv , libhound , libpcm ]
_extra_cflags = [ '-DNAME="hound"', ]
_src = files(
	'audio_data.c',
	'audio_device.c',
	'audio_sink.c',
	'audio_source.c',
	'connection.c',
	'hound.c',
	'hound_ctx.c',
	'iface.c',
	'main.c',
)

hound = executable(_name, _src,
	dependencies: _deps,
	c_args: _extra_cflags,
)
