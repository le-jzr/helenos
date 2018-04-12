_libname = 'ieee80211'
_deps = [ libc, libdrv , libnic , libcrypto ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/ieee80211.c',
	'src/ieee80211_impl.c',
	'src/ieee80211_iface_impl.c',
)

libieee80211 = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
