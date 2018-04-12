_libname = 'pcm'
_deps = [ libc ]
_extra_cflags = [ '-Iinclude/pcm', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/format.c',
)

libpcm = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
