_libname = 'arch'
_deps = []
_extra_cflags = [ '-fno-builtin' ]
_exports = libc_includes
_includes = _exports
_src = files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.S',
	'src/fibril.S',
	'src/tls.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
)

_startup_src = files('src/entry.S')

# TODO
_autocheck_headers = files(
	'include/libarch/fibril_context.h',
)

libarch = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
	sources : _startup_src,
)
