_name = 'download'
_deps = [ libc, libhttp , liburi ]
_extra_cflags = [ '-DRELEASE=' + h_release ]
_includes = include_directories('.')
_src = files(
	'main.c',
)

download = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
