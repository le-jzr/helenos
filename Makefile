#
# Copyright (c) 2006 Martin Decky
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

# Just for this Makefile. Sub-makes will run in parallel if requested.
.NOTPARALLEL:

CSCOPE = cscope
FORMAT = clang-format
CHECK = tools/check.sh

PYTHON_ENV = PYTHONDONTWRITEBYTECODE=y
CONFIG = $(PYTHON_ENV) tools/config.py
AUTOTOOL = $(PYTHON_ENV) tools/autotool.py

CONFIG_RULES = HelenOS.config

BUILD_DIR ?= build

SANDBOX = $(BUILD_DIR)/autotool

COMMON_MAKEFILE = $(BUILD_DIR)/Makefile.common
COMMON_HEADER = $(BUILD_DIR)/common.h
COMMON_HEADER_PREV = $(COMMON_HEADER).prev

CONFIG_MAKEFILE = $(BUILD_DIR)/Makefile.config
CONFIG_HEADER = $(BUILD_DIR)/config.h

ifeq ($(BUILD_DIR),build)
	KERNEL_BUILD_DIR = ../build/kernel
	USPACE_BUILD_DIR = ../build/uspace
	BOOT_BUILD_DIR = ../build/boot
else
	# TODO: It would be ideal if we could keep the path relative when
	#       it's provided as relative, but I'm not sure if Make can
	#       express the condition. It should be something like:
	#
	#   if ($(BUILD_DIR) starts with '/') {
	#           KERNEL_BUILD_DIR = $(BUILD_DIR)/kernel
	#   } else {
	#           KERNEL_BUILD_DIR = ../$(BUILD_DIR)/kernel
	#   }
	#
	KERNEL_BUILD_DIR = $(abspath $(BUILD_DIR))/kernel
	USPACE_BUILD_DIR = $(abspath $(BUILD_DIR))/uspace
	BOOT_BUILD_DIR = $(abspath $(BUILD_DIR))/boot
endif



.PHONY: all precheck cscope cscope_parts autotool config_auto config_default config distclean clean check releasefile release

all: $(COMMON_MAKEFILE) $(COMMON_HEADER) $(CONFIG_MAKEFILE) $(CONFIG_HEADER)
	cp -a $(COMMON_HEADER) $(COMMON_HEADER_PREV)
	mkdir -p $(BUILD_DIR)/kernel $(BUILD_DIR)/uspace $(BUILD_DIR)/boot
	$(MAKE) -r -C kernel PRECHECK=$(PRECHECK) BUILD_DIR=$(KERNEL_BUILD_DIR)
	$(MAKE) -r -C uspace PRECHECK=$(PRECHECK) BUILD_DIR=$(USPACE_BUILD_DIR)
	$(MAKE) -r -C boot PRECHECK=$(PRECHECK) BUILD_DIR=$(BOOT_BUILD_DIR)

precheck: clean
	$(MAKE) -r all PRECHECK=y

cscope:
	find abi kernel boot uspace -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE).out

cscope_parts:
	find abi -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_abi.out
	find kernel -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_kernel.out
	find boot -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_boot.out
	find uspace -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_uspace.out

format:
	find abi kernel boot uspace -type f -regex '^.*\.[ch]$$' | xargs $(FORMAT) -i -sort-includes -style=file

# Pre-integration build check
check: $(CHECK)
ifdef JOBS
	$(CHECK) -j $(JOBS)
else
	$(CHECK)
endif

# Autotool (detects compiler features)

autotool $(COMMON_MAKEFILE) $(COMMON_HEADER): $(CONFIG_MAKEFILE)
	$(AUTOTOOL) $(BUILD_DIR)
	-[ -f $(COMMON_HEADER_PREV) ] && diff -q $(COMMON_HEADER_PREV) $(COMMON_HEADER) && mv -f $(COMMON_HEADER_PREV) $(COMMON_HEADER)

# Build-time configuration

config_default $(CONFIG_MAKEFILE) $(CONFIG_HEADER): $(CONFIG_RULES)
	mkdir -p $(BUILD_DIR)
ifeq ($(HANDS_OFF),y)
	$(CONFIG) $(BUILD_DIR) $< hands-off $(PROFILE)
else
	$(CONFIG) $(BUILD_DIR) $< default $(PROFILE)
endif

config: $(CONFIG_RULES)
	$(CONFIG) $(BUILD_DIR) $<

random-config: $(CONFIG_RULES)
	$(CONFIG) $(BUILD_DIR) $< random

# Release files

releasefile: all
	$(MAKE) -r -C release releasefile

release:
	$(MAKE) -r -C release release

# Cleaning

distclean: clean
	rm -f $(CSCOPE).out $(COMMON_MAKEFILE) $(COMMON_HEADER) $(COMMON_HEADER_PREV) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) tools/*.pyc tools/checkers/*.pyc release/HelenOS-*

clean:
	rm -rf $(SANDBOX)
	$(MAKE) -r -C kernel clean BUILD_DIR=$(KERNEL_BUILD_DIR)
	$(MAKE) -r -C uspace clean BUILD_DIR=$(USPACE_BUILD_DIR)
	$(MAKE) -r -C boot clean BUILD_DIR=$(BOOT_BUILD_DIR)

-include Makefile.local
