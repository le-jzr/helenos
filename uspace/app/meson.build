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

essential = [
	'bdsh',
	'getterm',
	'kio',
	'vlaunch',
	'vterm',
]

foreach app : apps
	src = []
	test_src = []
	includes = []
	deps = []
	c_args = []
	link_args = []

	subdir(app)

	deps += libc

	if src.length() > 0
		executable(app + '.bin', src,
			include_directories: includes,
			dependencies: deps,
			c_args: c_args,
			link_args: link_args,
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('uspace', 'app', app),
		)
	endif

	if test_src.length() > 0
		executable('test_' + app, test_src,
			include_directories: includes,
			dependencies: [ deps, libpcut ],
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('uspace', 'app', app),
		)
	endif
endforeach