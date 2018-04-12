_libname = 'gui'
_deps = [ libc, libdraw , libsoftrend , libgraph ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'common.c',
	'button.c',
	'canvas.c',
	'connection.c',
	'grid.c',
	'label.c',
	'minimal.c',
	'terminal.c',
	'widget.c',
	'window.c',
)

libgui = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
