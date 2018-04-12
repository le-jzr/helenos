_name = 'kio'
_deps = [ libc, libclui ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'kio.c',
)

kio = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
