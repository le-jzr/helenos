_libname = 'math'
_deps = [ libc ]
_extra_cflags = [ '-Iarch/amd64/include', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'generic/acos.c',
	'generic/asin.c',
	'generic/atan.c',
	'generic/atan2.c',
	'generic/ceil.c',
	'generic/cosh.c',
	'generic/exp.c',
	'generic/fabs.c',
	'generic/floor.c',
	'generic/fmod.c',
	'generic/frexp.c',
	'generic/ldexp.c',
	'generic/log.c',
	'generic/log10.c',
	'generic/modf.c',
	'generic/pow.c',
	'generic/sinh.c',
	'generic/sqrt.c',
	'generic/tan.c',
	'generic/tanh.c',
	'generic/trig.c',
	'generic/trunc.c',
	'arch/amd64/src/sin.S',
	'arch/amd64/src/cos.S',
	'arch/amd64/src/trunc.S',
)

libmath = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
