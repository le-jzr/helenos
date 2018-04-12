_libname = 'label'
_deps = [ libc ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/dummy.c',
	'src/empty.c',
	'src/mbr.c',
	'src/gpt.c',
	'src/label.c',
)

liblabel = declare_dependency(
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
	'test/label.c',
)

test_liblabel = executable('test_liblabel', _test_src,
	include_directories: _includes,
	dependencies: [ liblabel, libpcut ],
)
