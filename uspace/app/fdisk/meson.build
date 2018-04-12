_name = 'fdisk'
_deps = [ libc, libclui , libfdisk ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'fdisk.c',
)

fdisk = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
