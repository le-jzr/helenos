#
# Copyright (c) 2005 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

## Configuration
#

ROOT_PATH = ..

VERSION_DEF = $(ROOT_PATH)/version

COMMON_MAKEFILE = $(ROOT_PATH)/Makefile.common
COMMON_HEADER = $(ROOT_PATH)/common.h
COMMON_HEADER_ARCH = arch/$(KARCH)/include/arch/common.h

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config
CONFIG_HEADER = $(ROOT_PATH)/config.h

-include $(VERSION_DEF)
-include $(COMMON_MAKEFILE)
-include $(CONFIG_MAKEFILE)

## Common names
#

RAW = kernel.raw
BIN = kernel.bin
JOB = kernel.job
DISASM = kernel.disasm
DUMP = kernel.dump
EMPTY_MAP = generic/src/debug/empty_map
SIZEOK_MAP = generic/src/debug/sizeok_map
REAL_MAP = generic/src/debug/real_map

GENMAP = tools/genmap.py
JOBFILE = $(ROOT_PATH)/tools/jobfile.py

LINK = arch/$(KARCH)/_link.ld

.PHONY: all clean

all: build.ninja
	ninja

clean:
	ninja -t clean

## Common compiler flags
#

INCLUDES = generic/include genarch/include arch/$(KARCH)/include ../abi/include
INCLUDES_FLAGS = $(addprefix -I,$(INCLUDES))

DEFS = -DKERNEL -DRELEASE=$(RELEASE) "-DCOPYRIGHT=$(COPYRIGHT)" "-DNAME=$(NAME)" -D__$(BITS)_BITS__ -D__$(ENDIANESS)__

COMMON_CFLAGS = $(INCLUDES_FLAGS) -O$(OPTIMIZATION) -imacros $(CONFIG_HEADER) \
	-ffreestanding -fno-builtin -nostdlib -nostdinc \
	-fexec-charset=UTF-8 -finput-charset=UTF-8 -fno-common \
	-fdebug-prefix-map=$(realpath $(ROOT_PATH))=.

GCC_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-unused-parameter \
	-Wmissing-prototypes -Werror-implicit-function-declaration \
	-Wwrite-strings -pipe

CLANG_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-unused-parameter \
	-Wno-missing-field-initializers -Wno-unused-command-line-argument \
	-Wmissing-prototypes -Werror-implicit-function-declaration \
	-Wwrite-strings -pipe -fno-stack-protector -fno-PIC

ifeq ($(CONFIG_DEBUG),y)
	COMMON_CFLAGS += -Werror
endif

ifeq ($(CONFIG_LTO),y)
	COMMON_CFLAGS += -flto
endif

ifeq ($(CONFIG_LINE_DEBUG),y)
	COMMON_CFLAGS += -g
endif

#
# Mind the mutual ordering with the inclusion of the arch Makefile.inc.
# AFLAGS and LFLAGS must be initialized before the inclusion.
#
AFLAGS =
LFLAGS = -n -T $(LINK) -M --fatal-warnings --warn-common

# FIXME: This condition is a workaround for issues #693 and #694.
ifneq ($(KARCH),sparc64)
ifneq ($(KARCH),mips32)
	AFLAGS += --fatal-warnings
endif
endif

#
# Mind the mutual ordering with the initialization of AFLAGS and LFLAGS.
# The arch Makefile.inc must be included after the initialization.
#
-include arch/$(KARCH)/Makefile.inc
-include genarch/Makefile.inc

## The at-sign
#
# The $(ATSIGN) variable holds the ASCII character representing the at-sign
# ('@') used in various $(AS) constructs (e.g. @progbits). On architectures that
# don't use '@' for starting a comment, $(ATSIGN) is merely '@'. However, on
# those that do use it for starting a comment (e.g. arm32), $(ATSIGN) must be
# defined as the percentile-sign ('%') in the architecture-dependent
# Makefile.inc.
#

ATSIGN ?= @

## Cross-platform assembly to start a symtab.data section
#

SYMTAB_SECTION = .section symtab.data, \"a\", $(ATSIGN)progbits;

## Compilation options
#

ifeq ($(COMPILER),clang)
	CFLAGS = $(COMMON_CFLAGS) $(CLANG_CFLAGS)
else
	CFLAGS = $(COMMON_CFLAGS) $(GCC_CFLAGS)
endif

INSTRUMENTATION = -finstrument-functions

## Generic kernel sources
#

GENERIC_SOURCES = \
	generic/src/adt/avl.c \
	generic/src/adt/bitmap.c \
	generic/src/adt/btree.c \
	generic/src/adt/cht.c \
	generic/src/adt/hash_table.c \
	generic/src/adt/list.c \
	generic/src/console/chardev.c \
	generic/src/console/console.c \
	generic/src/console/prompt.c \
	generic/src/cpu/cpu.c \
	generic/src/cpu/cpu_mask.c \
	generic/src/ddi/ddi.c \
	generic/src/ddi/irq.c \
	generic/src/debug/symtab.c \
	generic/src/debug/stacktrace.c \
	generic/src/debug/panic.c \
	generic/src/debug/debug.c \
	generic/src/interrupt/interrupt.c \
	generic/src/log/log.c \
	generic/src/main/main.c \
	generic/src/main/kinit.c \
	generic/src/main/uinit.c \
	generic/src/main/version.c \
	generic/src/main/shutdown.c \
	generic/src/proc/program.c \
	generic/src/proc/scheduler.c \
	generic/src/proc/thread.c \
	generic/src/proc/task.c \
	generic/src/proc/the.c \
	generic/src/syscall/syscall.c \
	generic/src/syscall/copy.c \
	generic/src/mm/km.c \
	generic/src/mm/reserve.c \
	generic/src/mm/frame.c \
	generic/src/mm/page.c \
	generic/src/mm/tlb.c \
	generic/src/mm/as.c \
	generic/src/mm/backend_anon.c \
	generic/src/mm/backend_elf.c \
	generic/src/mm/backend_phys.c \
	generic/src/mm/backend_user.c \
	generic/src/mm/slab.c \
	generic/src/lib/func.c \
	generic/src/lib/mem.c \
	generic/src/lib/memfnc.c \
	generic/src/lib/gsort.c \
	generic/src/lib/str.c \
	generic/src/lib/elf.c \
	generic/src/lib/ra.c \
	generic/src/lib/rd.c \
	generic/src/printf/printf_core.c \
	generic/src/printf/printf.c \
	generic/src/printf/snprintf.c \
	generic/src/printf/vprintf.c \
	generic/src/printf/vsnprintf.c \
	generic/src/time/clock.c \
	generic/src/time/timeout.c \
	generic/src/time/delay.c \
	generic/src/preempt/preemption.c \
	generic/src/synch/spinlock.c \
	generic/src/synch/condvar.c \
	generic/src/synch/mutex.c \
	generic/src/synch/semaphore.c \
	generic/src/synch/smc.c \
	generic/src/synch/smp_memory_barrier.c \
	generic/src/synch/waitq.c \
	generic/src/synch/futex.c \
	generic/src/synch/workqueue.c \
	generic/src/synch/rcu.c \
	generic/src/smp/ipi.c \
	generic/src/smp/smp.c \
	generic/src/smp/smp_call.c \
	generic/src/ipc/ipc.c \
	generic/src/ipc/sysipc.c \
	generic/src/ipc/sysipc_ops.c \
	generic/src/ipc/ops/conctmeto.c \
	generic/src/ipc/ops/concttome.c \
	generic/src/ipc/ops/dataread.c \
	generic/src/ipc/ops/datawrite.c \
	generic/src/ipc/ops/debug.c \
	generic/src/ipc/ops/pagein.c \
	generic/src/ipc/ops/sharein.c \
	generic/src/ipc/ops/shareout.c \
	generic/src/ipc/ops/stchngath.c \
	generic/src/ipc/ipcrsc.c \
	generic/src/ipc/irq.c \
	generic/src/ipc/event.c \
	generic/src/cap/cap.c \
	generic/src/security/perm.c \
	generic/src/sysinfo/sysinfo.c \
	generic/src/sysinfo/stats.c

## Kernel console support
#

ifeq ($(CONFIG_KCONSOLE),y)
GENERIC_SOURCES += \
	generic/src/console/kconsole.c \
	generic/src/console/cmd.c
endif

## Udebug interface sources
#

ifeq ($(CONFIG_UDEBUG),y)
GENERIC_SOURCES += \
	generic/src/ipc/kbox.c \
	generic/src/udebug/udebug.c \
	generic/src/udebug/udebug_ops.c \
	generic/src/udebug/udebug_ipc.c
endif

## Test sources
#

ifeq ($(CONFIG_TEST),y)
	CFLAGS += -Itest/
	GENERIC_SOURCES += \
		test/test.c \
		test/atomic/atomic1.c \
		test/btree/btree1.c \
		test/cht/cht1.c \
		test/avltree/avltree1.c \
		test/fault/fault1.c \
		test/mm/falloc1.c \
		test/mm/falloc2.c \
		test/mm/mapping1.c \
		test/mm/slab1.c \
		test/mm/slab2.c \
		test/synch/semaphore1.c \
		test/synch/semaphore2.c \
		test/synch/workqueue2.c \
		test/synch/workqueue3.c \
		test/synch/rcu1.c \
		test/print/print1.c \
		test/print/print2.c \
		test/print/print3.c \
		test/print/print4.c \
		test/print/print5.c \
		test/thread/thread1.c \
		test/smpcall/smpcall1.c
	
	ifeq ($(KARCH),mips32)
		GENERIC_SOURCES += test/debug/mips1.c
	else
		GENERIC_SOURCES += test/debug/mips1_skip.c
	endif
	
	ifeq ($(KARCH),ia64)
		GENERIC_SOURCES += test/mm/purge1.c
	else
		GENERIC_SOURCES += test/mm/purge1_skip.c
	endif
	
endif

## Sources where instrumentation is enabled
#

ifeq ($(CONFIG_TRACE),y)
	INSTRUMENTED_SOURCES = \
		generic/src/adt/btree.c \
		generic/src/cpu/cpu.c \
		generic/src/ddi/ddi.c \
		generic/src/interrupt/interrupt.c \
		generic/src/main/main.c \
		generic/src/main/kinit.c \
		generic/src/proc/the.c \
		generic/src/mm/frame.c \
		generic/src/mm/page.c \
		generic/src/mm/tlb.c \
		generic/src/mm/as.c \
		generic/src/mm/slab.c \
		generic/src/sysinfo/sysinfo.c \
		generic/src/console/kconsole.c
else
	INSTRUMENTED_SOURCES =
endif

AUTOGENS_AG := $(ARCH_AUTOGENS_AG) $(GENARCH_AUTOGENS_AG)

AS_CFLAGS := $(addprefix -Xassembler ,$(AFLAGS))
LD_CFLAGS :=

ifeq ($(COMPILER),clang)
	AS_CFLAGS += -mllvm -asm-macro-max-nesting-depth=1000
endif

ifeq ($(CONFIG_SYMTAB),y)
	SYMTAB_OBJECTS := $(REAL_MAP).o
else
	SYMTAB_OBJECTS :=
endif

ALL_MAKEFILES = Makefile $(VERSION_DEF) $(COMMON_MAKEFILE) $(CONFIG_MAKEFILE) arch/$(KARCH)/Makefile.inc genarch/Makefile.inc

ifeq ($(CONFIG_STRIP_BINARIES),y)
LFLAGS += --strip-all
endif

ALL_SOURCES = $(GENERIC_SOURCES) $(ARCH_SOURCES) $(GENARCH_SOURCES)
C_SOURCES = $(filter %.c, $(ALL_SOURCES))
UNINSTRUMENTED_SOURCES = $(filter-out $(INSTRUMENTED_SOURCES), $(C_SOURCES))
ASM_SOURCES = $(filter %.s, $(ALL_SOURCES))
PREASM_SOURCES = $(filter %.S, $(ALL_SOURCES))

ALMOST_ALL_OBJECTS = $(addsuffix .o,$(basename $(ALL_SOURCES))) $(EXTRA_OBJECTS)
ALL_OBJECTS = $(ALMOST_ALL_OBJECTS) $(SYMTAB_OBJECTS)

ifeq ($(CONFIG_LTO),y)
	CONFIG_LTO_DEF = -D__CONFIG_LTO__
else
	CONFIG_LTO_DEF = -U__CONFIG_LTO__
endif

build.ninja: build.ninja.in $(ALL_MAKEFILES)
	@echo "Generating build script for ninja."
	@$(CC) -E -P -undef -x c build.ninja.in -o $@.new \
		-D__KARCH__='$(KARCH)' \
		-D__CC__='$(CC)' \
		-D__LD__='$(LD)' \
		-D__AUTOGEN__='$(AUTOGEN)' \
		-D__CC_AUTOGEN__='$(CC_AUTOGEN)' \
		-D__LINKER_SCRIPT__='$(LINK)' \
		-D__LFLAGS__='$(LFLAGS)' \
		-D__LD_CFLAGS__='$(addprefix -Xlinker ,$(LFLAGS))' \
		-D__DEFS__='$(DEFS)' \
		-D__CFLAGS__='$(CFLAGS)' \
		-D__EXTRA_FLAGS__='$(EXTRA_FLAGS)' \
		-D__FPU_NO_CFLAGS__='$(FPU_NO_CFLAGS)' \
		-D__INSTRUMENTATION__='$(INSTRUMENTATION)' \
		-D__AS_CFLAGS__='$(AS_CFLAGS)' \
		-D__OBJCOPY__='$(OBJCOPY)' \
		-D__OBJDUMP__='$(OBJDUMP)' \
		-D__GENMAP__='$(GENMAP)' \
		$(CONFIG_LTO_DEF)

	@echo 'build $@: remake $@.in | $(ALL_MAKEFILES) $(AUTOGENS_AG:.ag=.ag.s.d)' >> $@.new
	@echo 'build $(BIN): objcopy $(RAW)' >> $@.new
	@echo '    flags = -O $(BFD)' >> $@.new
	@echo 'build $(DISASM): disasm $(RAW)' >> $@.new
	@echo 'build $(RAW) | $(RAW).map: link $(ALL_OBJECTS) | $$linker_script' >> $@.new
	@echo 'build $$linker_script: pp $$linker_script.in || ag-headers' >> $@.new
	@echo '    defines = $(INCLUDES_FLAGS) $(DEFS) -D__ASM__ -D__LINKER__ -imacros $(CONFIG_HEADER)' >> $@.new

	@echo 'build almost-all: phony $(ALMOST_ALL_OBJECTS)' >> $@.new

	@echo 'build $(EMPTY_MAP).bin: touch' >> $@.new
	@echo 'build $(EMPTY_MAP).o: pack $(EMPTY_MAP).bin' >> $@.new
	@echo '    section_name = "symtab.data"' >> $@.new

	@echo 'build $(SIZEOK_MAP).raw | $(SIZEOK_MAP).raw.map: link $(ALMOST_ALL_OBJECTS) $(EMPTY_MAP).o | $$linker_script' >> $@.new
	@echo 'build $(SIZEOK_MAP).syms: symbol_dump $(ALMOST_ALL_OBJECTS)' >> $@.new
	@echo 'build $(SIZEOK_MAP).bin: genmap $(SIZEOK_MAP).raw.map $(SIZEOK_MAP).syms' >> $@.new
	@echo 'build $(SIZEOK_MAP).o: pack $(SIZEOK_MAP).bin' >> $@.new
	@echo '    section_name = "symtab.data"' >> $@.new

	@# Do it once again, this time to get correct even the symbols
	@# on architectures that have bss after symtab
	@echo 'build $(REAL_MAP).raw | $(REAL_MAP).raw.map: link $(ALMOST_ALL_OBJECTS) $(SIZEOK_MAP).o | $$linker_script' >> $@.new
	@echo 'build $(REAL_MAP).syms: symbol_dump $(ALMOST_ALL_OBJECTS)' >> $@.new
	@echo 'build $(REAL_MAP).bin: genmap $(REAL_MAP).raw.map $(REAL_MAP).syms' >> $@.new
	@echo 'build $(REAL_MAP).o: pack $(REAL_MAP).bin' >> $@.new
	@echo '    section_name = "symtab.data"' >> $@.new

	@echo 'build ag-headers: phony $(AUTOGENS_AG:.ag=.h)' >> $@.new

	@$(foreach source,$(AUTOGENS_AG),echo 'build $(source).c: ag_probe $(source)' >> $@.new;)
	@$(foreach source,$(AUTOGENS_AG),echo 'build $(source).s.d: ag_deps $(source).c' >> $@.new; echo '    target = $(source).s' >> $@.new;)
	@$(foreach source,$(AUTOGENS_AG),echo 'build $(source).s: ag_cc $(source).c | $(source).s.d' >> $@.new;)
	@$(foreach source,$(AUTOGENS_AG),echo 'build $(source:.ag=.h): ag_generate $(source).s $(source)' >> $@.new;)

	@$(foreach source,$(UNINSTRUMENTED_SOURCES),echo 'build $(source:.c=.o): cc $(source) || ag-headers' >> $@.new;)
	@$(foreach source,$(INSTRUMENTED_SOURCES),echo 'build $(source:.c=.o): cc_instrumented $(source) || ag-headers' >> $@.new;)
	@$(foreach source,$(ASM_SOURCES),echo 'build $(source:.s=.o): asm $(source) || ag-headers' >> $@.new;)
	@$(foreach source,$(PREASM_SOURCES),echo 'build $(source:.S=.o): asm $(source) || ag-headers' >> $@.new;)
	mv $@.new $@

