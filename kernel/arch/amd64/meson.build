bfd_name = 'elf64-x86-64'
bfd_arch = 'i386:x86-64'
bfd = 'binary'

fpu_no_cflags = [ '-mno-sse', '-mno-sse2' ]
common_cflags += [ '-mcmodel=' + memory_model, '-mno-red-zone', '-fno-unwind-tables', '-fno-omit-frame-pointer' ]

bits = 64
endianness = 'LE'

if processor == 'opteron'
	common_cflags += '-march=opteron'
endif

arch_src = files(
	'src/fpu_context.c',
	'src/boot/multiboot.S',
	'src/boot/multiboot2.S',
	'src/boot/memmap.c',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/pm.c',
	'src/vreg.c',
	'src/kseg.c',
	'src/context.S',
	'src/ddi/ddi.c',
	'src/drivers/i8254.c',
	'src/drivers/i8259.c',
	'src/delay.S',
	'src/amd64.c',
	'src/bios/bios.c',
	'src/interrupt.c',
	'src/mm/km.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/asm.S',
	'src/cpu/cpu.c',
	'src/proc/scheduler.c',
	'src/proc/task.c',
	'src/proc/thread.c',
	'src/userspace.c',
	'src/syscall.c',
)

if config_smp
	arch_src += files(
		'src/smp/ap.S',
		'src/smp/apic.c',
		'src/smp/ipi.c',
		'src/smp/mps.c',
		'src/smp/smp_call.c',
		'src/smp/smp.c',
	)
endif

# TODO
arch_check_headers = files(
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
	'include/arch/kseg_struct.h',
	'include/arch/boot/memmap_struct.h',
)
