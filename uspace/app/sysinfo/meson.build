_name = 'sysinfo'
_deps = [ libc ]
_extra_cflags = []
_includes = []
_src = files(
	'sysinfo.c',
)

sysinfo = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
