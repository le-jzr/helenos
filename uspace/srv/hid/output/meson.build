_name = 'output'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'ctl/serial.c',
	'port/ega.c',
	'port/chardev.c',
	'proto/vt100.c',
	'output.c',
)

output = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
