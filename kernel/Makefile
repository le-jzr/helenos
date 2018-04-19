
## Common names
#

AUTOCHECK = $(realpath $(ROOT_PATH)/tools/autocheck.awk)
RAW = kernel.raw
BIN = kernel.bin
MAP = kernel.map
JOB = kernel.job
MAP_PREV = $(MAP).prev
DISASM = kernel.disasm
DUMP = kernel.dump
REAL_MAP = generic/src/debug/real_map

GENMAP = tools/genmap.py

LINK = arch/$(KARCH)/_link.ld
EMPTY_MAP = generic/src/debug/empty_map.o
SIZEOK_MAP = generic/src/debug/sizeok_map.o

.PHONY: all clean autogen_clean depend
.DELETE_ON_ERROR:

INSTRUMENTATION = -finstrument-functions

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

#ifeq ($(COMPILER),clang)
#	AS_CFLAGS += -mllvm -asm-macro-max-nesting-depth=1000
#endif

ifeq ($(CONFIG_SYMTAB),y)
	SYMTAB_OBJECTS := generic/src/debug/real_map.o
else
	SYMTAB_OBJECTS :=
endif

$(BIN): $(RAW)
	$(OBJCOPY) -O $(BFD) $< $@

$(DISASM): $(RAW)
ifeq ($(CONFIG_LINE_DEBUG),y)
	$(OBJDUMP) -d -S $< > $@
else
	$(OBJDUMP) -d $< > $@
endif

$(RAW): $(LINK) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(SYMTAB_OBJECTS)
	$(CC) $(DEFS) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$(MAP) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SYMTAB_OBJECTS)

$(LINK): $(LINK).in
	$(CC) $(DEFS) $(CFLAGS) -D__ASSEMBLER__ -D__LINKER__ -E -x c $< | grep -v "^\#" > $@

%.check.c: %.h $(AUTOCHECK)
	cd $(<D) && $(AUTOCHECK) $(<F) > $(@F)

%.o: %.S
	$(CC_JOB) -c -MD -MP $(DEFS) $(CFLAGS) $(AS_CFLAGS)

%.o: %.s
	$(CC_JOB) -c -MD -MP $(DEFS) $(CFLAGS) $(AS_CFLAGS)

%.o: %.c
	$(CC_JOB) -c -MD -MP $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) $(FPU_NO_CFLAGS) $(if $(findstring $<,$(INSTRUMENTED_SOURCES)),$(INSTRUMENTATION))

$(REAL_MAP).o: $(REAL_MAP).bin
	echo "$(AS_PROLOG)""$(SYMTAB_SECTION)"" .incbin \"$<\"" | $(CC) $(CFLAGS) $(AS_CFLAGS) -x assembler -c -o $@ -

$(REAL_MAP).bin: $(LINK) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS)
	echo "$(AS_PROLOG)""$(SYMTAB_SECTION)" | $(CC) $(CFLAGS) $(AS_CFLAGS) -x assembler -c -o $(EMPTY_MAP) -
	$(CC) $(DEFS) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(EMPTY_MAP)
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > $(DUMP)
	$(GENMAP) $(MAP_PREV) $(DUMP) $@

	# Do it once again, this time to get correct even the symbols
	# on architectures that have bss after symtab

	echo "$(AS_PROLOG)""$(SYMTAB_SECTION)"" .incbin \"$@\"" | $(CC) $(CFLAGS) $(AS_CFLAGS) -x assembler -c -o $(SIZEOK_MAP) -
	$(CC) $(DEFS) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SIZEOK_MAP)
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > $(DUMP)
	$(GENMAP) $(MAP_PREV) $(DUMP) $@
