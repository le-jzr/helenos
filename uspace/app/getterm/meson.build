_name = 'getterm'
_deps = [ libc ]
_extra_cflags = [ '-DRELEASE=' + h_release, '-DCOPYRIGHT=' + h_copyright, '-DNAME=' + h_name ]
_includes = include_directories('.')
_src = files(
	'getterm.c',
	'version.c',
	'welcome.c',
)

getterm = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
