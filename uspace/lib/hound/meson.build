_libname = 'hound'
_deps = [ libc, libpcm ]
_extra_cflags = [ '-Iinclude/hound', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/protocol.c',
	'src/client.c',
)

libhound = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
