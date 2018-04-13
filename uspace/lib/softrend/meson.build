_libname = 'softrend'
_deps = [ libc, libmath ]
_extra_cflags = [ ]
_exports = include_directories('.')
_includes = _exports
_src = files(
	'compose.c',
	'filter.c',
	'pixconv.c',
	'rectangle.c',
	'transform.c',
)

libsoftrend = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
