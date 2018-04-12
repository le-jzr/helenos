_libname = 'usb'
_deps = [ libc, libdrv ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/class.c',
	'src/dev.c',
	'src/debug.c',
	'src/dma_buffer.c',
	'src/dump.c',
	'src/port.c',
	'src/usb.c',
)

libusb = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
