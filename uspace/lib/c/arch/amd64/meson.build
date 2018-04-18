c_args = [ '-fno-builtin' ]
src = files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.S',
	'src/fibril.S',
	'src/tls.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
)

startup_src = files('src/entry.S')

# TODO
_autocheck_headers = files(
	'include/libarch/fibril_context.h',
)

libarch = declare_dependency(
	link_with: library('arch', src,
		include_directories: libc_includes,
		c_args: c_args,
	),
	sources: startup_src,
)
