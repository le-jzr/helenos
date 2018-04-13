_name = 'cmos-rtc'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'cmos-rtc.c',
)

cmos_rtc = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
