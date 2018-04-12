_name = 'taskdump'
_deps = [ libc ]
_extra_cflags = [ '-Iinclude', ]
_includes = include_directories('.')
_src = files(
	'elf_core.c',
	'fibrildump.c',
	'taskdump.c',
	'symtab.c',
)

taskdump = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
