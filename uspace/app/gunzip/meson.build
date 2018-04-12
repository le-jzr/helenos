_name = 'gunzip'
_deps = [ libc, libcompress ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'gunzip.c',
)

gunzip = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
