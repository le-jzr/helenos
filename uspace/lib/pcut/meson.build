_libname = 'pcut'
_deps = []
_extra_cflags = [ '-D__helenos__' ]
_exports = include_directories('include')
_includes = [ _exports, libc_includes ]
_src = files(
	'src/os/helenos.c',
	'src/assert.c',
	'src/list.c',
	'src/main.c',
	'src/print.c',
	'src/report/report.c',
	'src/report/tap.c',
	'src/report/xml.c',
	'src/run.c',
)

libpcut = declare_dependency(
	link_with: static_library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)

# TODO: self tests