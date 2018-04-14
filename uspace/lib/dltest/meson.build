_libname = 'dltest'
_deps = [ libc ]
_extra_cflags = [ ]
_exports = include_directories('.')
_includes = _exports
_src = files(
	'dltest.c',
)

libdltest_includes = _includes

libdltest = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
