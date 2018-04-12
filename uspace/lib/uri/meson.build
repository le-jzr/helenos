_libname = 'uri'
_deps = [ libc ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'uri.c',
)

liburi = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)

_test_src = files(
	'test/main.c',
	'test/parser.c',
)

test_liburi = executable('test_liburi', _test_src,
	include_directories: _includes,
	dependencies: [ liburi, libpcut ],
)
