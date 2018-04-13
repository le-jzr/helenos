_libname = 'http'
_deps = [ libc ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/http.c',
	'src/headers.c',
	'src/request.c',
	'src/response.c',
	'src/receive-buffer.c',
)

libhttp = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
