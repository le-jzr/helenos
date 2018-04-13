_libname = 'drv'
_deps = [ libc, libpcm ]
_extra_cflags = [ ]
_exports = include_directories('include')
_includes = [ _exports, include_directories('generic/private') ]
_src = files(
	'generic/driver.c',
	'generic/dev_iface.c',
	'generic/interrupt.c',
	'generic/log.c',
	'generic/logbuf.c',
	'generic/remote_audio_mixer.c',
	'generic/remote_audio_pcm.c',
	'generic/remote_hw_res.c',
	'generic/remote_pio_window.c',
	'generic/remote_nic.c',
	'generic/remote_ieee80211.c',
	'generic/remote_usb.c',
	'generic/remote_pci.c',
	'generic/remote_usbdiag.c',
	'generic/remote_usbhc.c',
	'generic/remote_usbhid.c',
	'generic/remote_clock_dev.c',
	'generic/remote_led_dev.c',
	'generic/remote_battery_dev.c',
	'generic/remote_ahci.c',
)

libdrv = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
