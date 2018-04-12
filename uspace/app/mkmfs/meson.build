_name = 'mkmfs'
_deps = [ libc, libblock , libminix ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'mkmfs.c',
)

mkmfs = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
