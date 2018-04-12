_libname = 'nic'
_deps = [ libc, libdrv ]
_extra_cflags = [ '-DLIBNIC_INTERNAL', ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'src/nic_driver.c',
	'src/nic_ev.c',
	'src/nic_addr_db.c',
	'src/nic_rx_control.c',
	'src/nic_wol_virtues.c',
	'src/nic_impl.c',
)

libnic = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
