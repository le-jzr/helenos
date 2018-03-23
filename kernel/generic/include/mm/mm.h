/*
 * Copyright (c) 2007 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef KERN_MM_H_
#define KERN_MM_H_

#include <assert.h>
#include <stdbool.h>

enum {
	PAGE_PT_BIT = 1 << 0,
	PAGE_READ_BIT = 1 << 1,
	PAGE_WRITE_BIT = 1 << 2,
	PAGE_EXEC_BIT = 1 << 3,

	PAGE_DISCRIMINANT_BITS =
	    PAGE_PT_BIT | PAGE_READ_BIT | PAGE_WRITE_BIT | PAGE_EXEC_BIT,

	PAGE_GLOBAL_BIT = 1 << 6,
	PAGE_CACHEABLE_BIT = 1 << 7,
	PAGE_NOT_CACHEABLE_BIT = 1 << 8,
	PAGE_USER_BIT = 1 << 9,
	PAGE_KERNEL_BIT = 1 << 10,

	PAGE_MAX_WIDTH_SHIFT = 11,
};

/* Discriminant, exactly one of the following seven must be set for
 * the flags to be valid. Furthermore, the first two are only legal
 * for setting individual page table entries. Setting an entry
 * to PAGE_NOT_PRESENT renders the entry eligible for removal.
 * In an earlier iteration of this interface, page could be not
 * present but valid, preventing removal. This has been changed, and
 * if future iterations allow kernel to hide data (e.g. swap identifiers)
 * in page tables, it should be achieved by adding a separate discriminant.
 */
typedef enum page_discriminant {
	PAGE_NOT_PRESENT        = 0,
	PAGE_NEXT_LEVEL_PT      = PAGE_PT_BIT,
	PAGE_EXECUTE_ONLY       = PAGE_EXEC_BIT,
	PAGE_READ_ONLY          = PAGE_READ_BIT,
	PAGE_READ_EXECUTE       = PAGE_READ_BIT | PAGE_EXEC_BIT,
	PAGE_READ_WRITE         = PAGE_READ_BIT | PAGE_WRITE_BIT,
	PAGE_READ_WRITE_EXECUTE = PAGE_READ_BIT | PAGE_WRITE_BIT | PAGE_EXEC_BIT,
} page_discriminant_t;

static inline page_discriminant_t PAGE_DISCRIMINANT(page_flags_t flags)
{
	return (page_discriminant_t) (flags & PAGE_DISCRIMINANT_BITS);
}

static inline int PAGE_GET_MAX_WIDTH(page_flags_t flags)
{
	return flags >> PAGE_MAX_WIDTH_SHIFT;
}

static inline page_flags_t PAGE_LARGE(int max_width)
{
	return max_width << PAGE_MAX_WIDTH_SHIFT;
}

/* Global page. Can be combined with anything except PAGE_NOT_PRESENT.
 * PAGE_GLOBAL on non-leaf entry means all the leaf entries under it are global,
 * even if they don't have the PAGE_GLOBAL flag themselves.
 */
#define PAGE_GLOBAL  PAGE_GLOBAL_BIT

/* Cacheability.
 * Platform-independent code should always use PAGE_CACHEABLE for normal memory,
 * and PAGE_NOT_CACHEABLE for I/O memory.
 * In particular, setting PAGE_NOT_CACHEABLE on normal memory does not prevent
 * caching on all platforms. You have been warned.
 * Exactly one must be present for leaf pages.
 * None may be present for non-leaf entries.
 */
#define PAGE_CACHEABLE      PAGE_CACHEABLE_BIT
#define PAGE_NOT_CACHEABLE  PAGE_NOT_CACHEABLE_BIT

/* Protection.
 * PAGE_USER for memory accessible to userspace programs, PAGE_KERNEL for
 * memory accessible only to the kernel. Note that on some platforms,
 * PAGE_USER pages are accessible to kernel, while on others, they are not.
 * For non-leaf entries, PAGE_USER means that all of the lower-level pages
 * are PAGE_USER, likewise with PAGE_KERNEL. Exactly one of these two must be
 * used for leaf entries, but they may be omitted for non-leaf entries.
 */
#define PAGE_USER                 PAGE_USER_BIT
#define PAGE_KERNEL               PAGE_KERNEL_BIT

/* Permission check macros. These assume that `flags` are a valid combination
 * of above constants.
 */
#define PAGE_IS_PRESENT(flags)     (PAGE_DISCRIMINANT(flags) != PAGE_NOT_PRESENT)
#define PAGE_IS_READABLE(flags)    (((flags) & PAGE_READ_BIT) != 0)
#define PAGE_IS_WRITABLE(flags)    (((flags) & PAGE_WRITE_BIT) != 0)
#define PAGE_IS_EXECUTABLE(flags)  (((flags) & PAGE_EXEC_BIT) != 0)

/** A convenience macro for constructing a valid set of flags from
 *  a platform-specific representation.
 */
#define PAGE_FLAGS(...) _PAGE_FLAGS((_page_flags_t) { __VA_ARGS__ })

typedef struct {
	bool present;
	bool next_level;
	bool read;
	bool write;
	bool execute;
	bool global;
	bool cacheable;
	bool user_only;
	bool kernel_only;
} _page_flags_t;

static inline unsigned _PAGE_FLAGS(_page_flags_t pf) {
	if (!pf.present)
		return PAGE_NOT_PRESENT;

	unsigned int flags = 0;
	flags |= (pf.global ? PAGE_GLOBAL : 0);

	if (pf.next_level) {
		if (pf.kernel_only)
			flags |= PAGE_KERNEL;
		else
			flags |= (pf.user_only ? PAGE_USER : 0);
		return PAGE_NEXT_LEVEL_PT | flags;
	}

	flags |= (pf.cacheable ? PAGE_CACHEABLE : PAGE_NOT_CACHEABLE);
	flags |= (pf.kernel_only ? PAGE_KERNEL : PAGE_USER);
	flags |= (pf.read ? _PAGE_READ : 0);
	flags |= (pf.write ? (_PAGE_READ | _PAGE_WRITE) : 0);
	flags |= (pf.execute ? _PAGE_EXEC : 0);

	if (!pf.write && !pf.execute) {
		// If none are set, assume read is implied.
		flags |= _PAGE_READ;
	}
	return flags;
}

static inline bool PAGE_FLAGS_VALID(unsigned flags) {
	/* Empty entry supports no flags. */
	if (flags & PAGE_NOT_PRESENT)
		return flags == PAGE_NOT_PRESENT;

	/* PAGE_USER and PAGE_KERNEL are mutually exclusive. */
	if ((flags & PAGE_USER) && (flags & PAGE_KERNEL))
		return false;

	/* Check allowed flags for non-leaf entry. */
	if (flags & PAGE_NEXT_LEVEL_PT)
		return flags == (flags & (PAGE_NEXT_LEVEL_PT | PAGE_GLOBAL | PAGE_USER | PAGE_KERNEL));

	/* Leaf entries only. */

	/* Check that at least one permission is set. */
	if (!(flags & (_PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)))
		return false;

	/* Check that write implies read. */
	if ((flags & _PAGE_WRITE) && !(flags & _PAGE_READ))
		return false;

	/* One of PAGE_USER and PAGE_KERNEL must be used. */
	if (!(flags & (PAGE_USER | PAGE_KERNEL)))
		return false;

	/* One of PAGE_CACHEABLE and PAGE_NOT_CACHEABLE must be used. */
	if ((flags & PAGE_CACHEABLE) && (flags & PAGE_NOT_CACHEABLE))
		return false;
	if (!(flags & (PAGE_CACHEABLE | PAGE_NOT_CACHEABLE)))
		return false;

	return true;
}

#endif

/** @}
 */
