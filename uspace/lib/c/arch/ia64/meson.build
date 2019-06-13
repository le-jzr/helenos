arch_src = [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.S',
	'src/fibril.S',
	'src/tls.c',
	'src/ddi.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
)

arch_start_src = files('src/crt0.S')
