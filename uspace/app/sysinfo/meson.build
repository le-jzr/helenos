_name = 'sysinfo'
_deps = [ libc ]
_extra_cflags = [ '-Iinclude', ]
_includes = include_directories('.')
_src = files(
	'sysinfo.c',
)

sysinfo = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
