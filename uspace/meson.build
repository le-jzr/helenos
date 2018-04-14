project(
    'HelenOS userland',
    [ 'c' ],
    default_options : ['buildtype=plain', 'c_std=gnu99', 'warning_level=3', 'werror=true', 'b_staticpic=false', 'default_library=static' ],
    version : '0.7.1-git',
)

unzip = find_program('unzip')
mkarray = find_program('../tools/mkarray_for_meson.sh')
#config_header = files('../config.h')

add_project_link_arguments('-Wl,--fatal-warnings', '-Wl,--warn-common', language : [ 'c', 'cpp' ])
add_project_arguments('-D_HELENOS_SOURCE', '-Wa,--fatal-warnings', language : [ 'c', 'cpp' ])

h_arch = meson.get_cross_property('h_arch')

# FIXME
h_release = meson.project_version()
h_copyright = 'Copyright (c) 2001-2018 HelenOS project'
h_name = 'Enlightened Zeal'

subdir('meson/warnings')

# Redefined in libarch, if necessary.
# Used in a few places where we generate custom assembly.
as_prolog = ''

# There's a bit of a cyclic dependency between libc and libpcut due to the
# way code is organized.
libc_includes = include_directories(
	'lib/c/include',
	'lib/c/arch/' + h_arch + '/include',
	'../abi/include',
)
subdir('lib/pcut')
subdir('lib/c')

# XXX: It's important that dependencies are listed before libs that depend on them.
libs = [
	'block',
	'clui',
	'bithenge',
	'compress',
	'math',
	'softrend',
	'graph',
	'draw',
	'fmtutil',
	'gui',
	'scsi',
	'http',
	'uri',
	'fdisk',
	'pcm',
	'drv',
	'usb',
	'usbdev',
	'usbhid',
	'usbvirt',
	'minix',
	'trackmod',
	'hound',
	'label',
	'fs',
	'crypto',
	'ext4',
	'nettl',
	'usbhost',
	'nic',
	'ieee80211',
	'dltest',
]

# TODO: dltest dltests
apps = [
	'barber',
	'bdsh',
	'bithenge',
	'blkdump',
	'bnchmark',
	'corecfg',
	'date',
	'devctl',
	'df',
	'dltest',
	'dltests',
	'dnscfg',
	'dnsres',
	'download',
	'edit',
	'fdisk',
	'fontviewer',
	'getterm',
	'gunzip',
	'inet',
	'init',
	'kill',
	'killall',
	'kio',
	'loc',
	'logset',
	'mixerctl',
	'mkbd',
	'mkexfat',
	'mkfat',
	'mkmfs',
	'modplay',
	'netecho',
	'nic',
	'nterm',
	'ping',
	'pkg',
	'rcubench',
	'rcutest',
	'redir',
	'sbi',
	'sportdmp',
	'stats',
	'sysinfo',
	'sysinst',
	'taskdump',
	'tester',
	'testread',
	'testwrit',
	'tetris',
	'tmon',
	'top',
	'trace',
	'untar',
	'usbinfo',
	'vcalc',
	'vdemo',
	'viewer',
	'vlaunch',
	'vterm',
	'vuhid',
	'wavplay',
	'websrv',
	'wifi_supplicant',
]

srvs = [
	'audio/hound',
	'bd/file_bd',
	'bd/rd',
	'bd/sata_bd',
	'bd/vbd',
	'clipboard',
	'devman',
	'fs/cdfs',
	'fs/exfat',
	'fs/ext4fs',
	'fs/fat',
	'fs/locfs',
	'fs/mfs',
	'fs/tmpfs',
	'fs/udf',
	'hid/compositor',
	'hid/console',
	'hid/input',
	'hid/isdv4_tablet',
	'hid/output',
	'hid/remcons',
	'hid/rfb',
	'hid/s3c24xx_ts',
	'hw/char/s3c24xx_uart',
	'klog',
	'loader',
	'locsrv',
	'logger',
	'net/dhcp',
	'net/dnsrsrv',
	'net/ethip',
	'net/inetsrv',
	'net/loopip',
	'net/nconfsrv',
	'net/slip',
	'net/tcp',
	'net/udp',
	'ns',
	'taskmon',
	'test/chardev-test',
	'vfs',
	'volsrv',
]

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

foreach d : libs
	subdir('lib/' + d)
endforeach

foreach d : apps
	subdir('app/' + d)
endforeach

foreach d : srvs
	subdir('srv/' + d)
endforeach

foreach d : drvs
	subdir('drv/' + d)
endforeach