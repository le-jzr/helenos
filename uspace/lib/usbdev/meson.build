_libname = 'usbdev'
_deps = [ libc, libusb , libdrv ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/altiface.c',
	'src/driver.c',
	'src/devdrv.c',
	'src/devpoll.c',
	'src/dp.c',
	'src/pipes.c',
	'src/pipesinit.c',
	'src/recognise.c',
	'src/request.c',
)

libusbdev = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
