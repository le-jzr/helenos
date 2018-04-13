_name = 'loader'
_deps = [ libc ]
_src = files(
	'main.c',
)

loader = executable(_name, _src,
	dependencies: _deps,
	link_args: [ '-static', '-Wl,--Ttext-segment=0x70000000' ]
)
