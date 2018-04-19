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
	'init',
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

init = [
	'bd/rd',
	'fs/' + config_rdfmt,
	'init',
	'loader',
	'locsrv',
	'logger',
	'ns',
	'vfs',
]

essential = [
	'devman',
	'fs/locfs',
	'hid/input',
	'hid/output',
	'hid/compositor',
	'hid/console',
	'klog',
]


foreach d : srvs
	subdir(d)
endforeach

foreach srv : srvs
	src = []
	test_src = []
	includes = []
	deps = []
	c_args = []
	link_args = []

	subdir(srv)

	if init.contains(srv)
		link_args += [ '-static' ]
	endif

	deps += libc

	if src.length() > 0
		executable(srv.underscorify() + '.bin', src,
			include_directories: includes,
			dependencies: deps,
			c_args: c_args,
			link_args: link_args,
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('srv', srv),
		)
	endif

	if test_src.length() > 0
		executable('test_' + srv.underscorify(), test_src,
			include_directories: includes,
			dependencies: [ deps, libpcut ],
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('srv', srv),
		)
	endif
endforeach