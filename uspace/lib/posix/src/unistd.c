/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file Miscellaneous standard definitions.
 */

#include "internal/common.h"
#include "posix/unistd.h"

#include <errno.h>

#include "posix/string.h"
#include "posix/fcntl.h"

#include "libc/task.h"
#include "libc/thread.h"
#include "libc/stats.h"
#include "libc/malloc.h"
#include "libc/vfs/vfs.h"

#include <libarch/config.h>

/* Array of environment variable strings (NAME=VALUE). */
char **environ = NULL;

/**
 * Sleep for the specified number of seconds.
 *
 * Note that POSIX allows this call to be interrupted and then the return
 * value represents remaining seconds for the sleep. HelenOS does not offer
 * such functionality and thus always the whole sleep is taken.
 *
 * @param seconds Number of seconds to sleep.
 * @return Always 0 on HelenOS.
 */
unsigned int sleep(unsigned int seconds)
{
	return thread_sleep(seconds);
}

/**
 * Get current user name.
 *
 * @return User name (static) string or NULL if not found.
 */
char *getlogin(void)
{
	/* There is currently no support for user accounts in HelenOS. */
	return (char *) "user";
}

/**
 * Get current user name.
 *
 * @param name Pointer to a user supplied buffer.
 * @param namesize Length of the buffer.
 * @return Zero on success, error code otherwise.
 */
int getlogin_r(char *name, size_t namesize)
{
	/* There is currently no support for user accounts in HelenOS. */
	if (namesize >= 5) {
		strcpy(name, (char *) "user");
		return 0;
	} else {
		errno = ERANGE;
		return -1;
	}
}

/**
 * Get the pathname of the current working directory.
 *
 * @param buf Buffer into which the pathname shall be put.
 * @param size Size of the buffer.
 * @return Buffer pointer on success, NULL on failure.
 */
char *getcwd(char *buf, size_t size)
{
	if (failed(vfs_cwd_get(buf, size)))
		return NULL;
	return buf;
}

/**
 * Change the current working directory.
 *
 * @param path New working directory.
 */
int chdir(const char *path)
{
	if (failed(vfs_cwd_set(path)))
		return -1;
	return 0;
}

/**
 * Determine the page size of the current run of the process.
 *
 * @return Page size of the process.
 */
int getpagesize(void)
{
	return PAGE_SIZE;
}

/**
 * Get the process ID of the calling process.
 *
 * @return Process ID.
 */
pid_t getpid(void)
{
	return task_get_id();
}

/**
 * Get the real user ID of the calling process.
 *
 * @return User ID.
 */
uid_t getuid(void)
{
	/* There is currently no support for user accounts in HelenOS. */
	return 0;
}

/**
 * Get the real group ID of the calling process.
 *
 * @return Group ID.
 */
gid_t getgid(void)
{
	/* There is currently no support for user accounts in HelenOS. */
	return 0;
}

/**
 * Remove a directory.
 *
 * @param path Directory pathname.
 * @return Zero on success, -1 otherwise.
 */
int rmdir(const char *path)
{
	// FIXME: Follow POSIX spec.
	if (failed(vfs_unlink_path(path)))
		return -1;
	else
		return 0;
}

/**
 * Remove a link to a file.
 *
 * @param path File pathname.
 * @return Zero on success, -1 otherwise.
 */
int unlink(const char *path)
{
	// FIXME: Follow POSIX spec.
	if (failed(vfs_unlink_path(path)))
		return -1;
	else
		return 0;
}

/**
 * Determine accessibility of a file.
 *
 * @param path File to check accessibility for.
 * @param amode Either check for existence or intended access mode.
 * @return Zero on success, -1 otherwise.
 */
int access(const char *path, int amode)
{
	if (amode == F_OK || (amode & (X_OK | W_OK | R_OK))) {
		/* HelenOS doesn't support permissions, permission checks
		 * are equal to existence check.
		 *
		 * Check file existence by attempting to open it.
		 */
		int fd = open(path, O_RDONLY);
		if (fd < 0)
			return -1;
		close(fd);
		return 0;
	} else {
		/* Invalid amode argument. */
		errno = EINVAL;
		return -1;
	}
}

/**
 * Get configurable system variables.
 *
 * @param name Variable name.
 * @return Variable value.
 */
long sysconf(int name)
{
	long clk_tck = 0;
	size_t cpu_count = 0;
	stats_cpu_t *cpu_stats = stats_get_cpus(&cpu_count);
	if (cpu_stats && cpu_count > 0) {
		clk_tck = ((long) cpu_stats[0].frequency_mhz) * 1000000L;
	}
	if (cpu_stats) {
		free(cpu_stats);
		cpu_stats = 0;
	}

	long phys_pages = 0;
	long avphys_pages = 0;
	stats_physmem_t *mem_stats = stats_get_physmem();
	if (mem_stats) {
		phys_pages = (long) (mem_stats->total / getpagesize());
		avphys_pages = (long) (mem_stats->free / getpagesize());
		free(mem_stats);
		mem_stats = 0;
	}

	switch (name) {
	case _SC_PHYS_PAGES:
		return phys_pages;
	case _SC_AVPHYS_PAGES:
		return avphys_pages;
	case _SC_PAGESIZE:
		return getpagesize();
	case _SC_CLK_TCK:
		return clk_tck;
	default:
		errno = EINVAL;
		return -1;
	}
}

/**
 *
 * @param path
 * @param name
 * @return
 */
long pathconf(const char *path, int name)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
	return -1;
}

/**
 *
 * @return
 */
pid_t fork(void)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
	return -1;
}

/**
 *
 * @param path
 * @param argv
 * @return
 */
int execv(const char *path, char *const argv[])
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
	return -1;
}

/**
 *
 * @param file
 * @param argv
 * @return
 */
int execvp(const char *file, char *const argv[])
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
	return -1;
}

unsigned int alarm(unsigned int seconds)
{
	not_implemented();
	return 0;
}

/** @}
 */
