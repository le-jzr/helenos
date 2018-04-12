_libname = 'softfloat'
_deps = [ libc ]
_extra_cflags = [ '-I../../lib/math/include',  '-I../../lib/math/arch/amd64/include', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'common.c',
	'add.c',
	'sub.c',
	'div.c',
	'mul.c',
	'neg.c',
	'comparison.c',
	'conversion.c',
)

libsoftfloat = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
