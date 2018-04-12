_name = 'rtl8169'
_deps = [ libc, libdrv , libnic ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'driver.c',
)

rtl8169 = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
