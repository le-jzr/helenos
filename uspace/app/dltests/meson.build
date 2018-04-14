_name = 'dltests'
_deps = [ libc ]
_src = files(
	'dltests.c',
)

dltests = executable(_name, _src,
	include_directories: libdltest_includes,
	dependencies: _deps,
	link_args: [ '-Wl,-Bdynamic' ],
)
