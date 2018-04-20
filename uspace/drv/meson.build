drvs = [
	'audio/hdaudio',
	'audio/sb16',
	'root/root',
	'root/virt',
	'block/ahci',
	'block/ata_bd',
	'block/ddisk',
	'block/usbmast',
	'bus/adb/cuda_adb',
	'bus/isa',
	'bus/pci/pciintel',
	'bus/usb/ehci',
	'bus/usb/ohci',
	'bus/usb/uhci',
	'bus/usb/usbdiag',
	'bus/usb/usbflbk',
	'bus/usb/usbhub',
	'bus/usb/usbmid',
	'bus/usb/vhc',
	'bus/usb/xhci',
	'char/i8042',
	'char/msim-con',
	'char/ns8250',
	'char/pl050',
	'char/ski-con',
	'char/sun4v-con',
	'fb/amdm37x_dispc',
	'fb/kfb',
	'hid/adb-kbd',
	'hid/adb-mouse',
	'hid/atkbd',
	'hid/ps2mouse',
	'hid/usbhid',
	'hid/xtkbd',
	'intctl/apic',
	'intctl/i8259',
	'intctl/icp-ic',
	'intctl/obio',
	'nic/ne2k',
	'nic/e1k',
	'nic/rtl8139',
	'nic/rtl8169',
	'nic/ar9271',
	'platform/amdm37x',
	'platform/icp',
	'platform/mac',
	'platform/malta',
	'platform/msim',
	'platform/pc',
	'platform/ski',
	'platform/sun4u',
	'platform/sun4v',
	'test/test1',
	'test/test2',
	'test/test3',
	'time/cmos-rtc',
]

essential = [
	'root/root',
	'root/virt',
	'fb/kfb',
]

foreach drv : drvs
	src = []
	test_src = []
	includes = []
	deps = []
	c_args = []
	link_args = []

	subdir(drv)

	deps += [ libc, libdrv ]

	dirname = drv

	drv = drv.split('/')
	drv = drv[drv.length()-1]

	if src.length() > 0
		executable(drv + '.bin', src,
			include_directories: includes,
			dependencies: deps,
			c_args: c_args,
			link_args: link_args,
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('uspace', 'drv', dirname),
		)
	endif

	if test_src.length() > 0
		executable('test_' + drv, test_src,
			include_directories: includes,
			dependencies: [ deps, libpcut ],
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('uspace', 'drv', dirname),
		)
	endif
endforeach