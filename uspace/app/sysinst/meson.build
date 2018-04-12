_name = 'sysinst'
_deps = [ libc, libblock , libfdisk ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'futil.c',
	'sysinst.c',
)

sysinst = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
