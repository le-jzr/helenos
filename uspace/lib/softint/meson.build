_libname = 'softint'
_deps = [ libc ]
_extra_cflags = [ '-Iinclude', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'generic/bits.c',
	'generic/comparison.c',
	'generic/division.c',
	'generic/multiplication.c',
	'generic/shift.c',
)

libsoftint = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
