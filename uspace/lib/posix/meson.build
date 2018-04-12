_libname = 'posix'
_deps = [ libc ]
_extra_cflags = [ '-Iinclude/',  '-fno-builtin', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/ctype.c',
	'src/dlfcn.c',
	'src/fcntl.c',
	'src/fnmatch.c',
	'src/locale.c',
	'src/pthread/condvar.c',
	'src/pthread/keys.c',
	'src/pthread/mutex.c',
	'src/pthread/threads.c',
	'src/pwd.c',
	'src/signal.c',
	'src/stdio.c',
	'src/stdio/scanf.c',
	'src/stdlib.c',
	'src/stdlib/strtold.c',
	'src/string.c',
	'src/strings.c',
	'src/sys/mman.c',
	'src/sys/stat.c',
	'src/sys/wait.c',
	'src/time.c',
	'src/unistd.c',
)

libposix = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)

_test_src = files(
	'test/main.c',
	'test/scanf.c',
)

test_libposix = executable('test_libposix', _test_src,
	include_directories: _includes,
	dependencies: [ libposix, libpcut ],
)
