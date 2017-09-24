#
# Copyright (c) 2012 Frantisek Princ
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

USPACE_PREFIX = ../..
LIBRARY = libext4
LIBS = block crypto fs

EXPORT_HEADERS = \
	ext4/balloc.h \
	ext4/bitmap.h \
	ext4/block_group.h \
	ext4/directory.h \
	ext4/directory_index.h \
	ext4/extent.h \
	ext4/filesystem.h \
	ext4/fstypes.h \
	ext4/hash.h \
	ext4/ialloc.h \
	ext4/inode.h \
	ext4/ops.h \
	ext4/superblock.h \
	ext4/types.h

SOURCES = \
	src/balloc.c \
	src/bitmap.c \
	src/block_group.c \
	src/directory.c \
	src/directory_index.c \
	src/extent.c \
	src/filesystem.c \
	src/hash.c \
	src/ialloc.c \
	src/inode.c \
	src/ops.c \
	src/superblock.c

include $(USPACE_PREFIX)/Makefile.common
