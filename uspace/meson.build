objdump = find_program('objdump')
disassemble = true

subdir('lib')
subdir('app')
subdir('srv')
subdir('drv')

init = [
	'app/init',
	'srv/bd/rd',
	'srv/fs/' + CONFIG_RDFMT,
	'srv/loader',
	'srv/locsrv',
	'srv/logger',
	'srv/ns',
	'srv/vfs',
]

essential = [
	'app/bdsh',
	'app/getterm',
	'app/kio',
	'app/vlaunch',
	'app/vterm',

	'srv/devman',
	'srv/fs/locfs',
	'srv/hid/input',
	'srv/hid/output',
	'srv/hid/compositor',
	'srv/hid/console',
	'srv/klog',

	'drv/root/root',
	'drv/root/virt',
	'drv/fb/kfb',
]

dirs = []

foreach app : apps
	dirs += [ join_paths('app', app) ]
endforeach

foreach srv : srvs
	dirs += [ join_paths('srv', srv) ]
endforeach

foreach drv : drvs
	dirs += [ join_paths('drv', drv) ]
endforeach

foreach dir : dirs
	src = []
	test_src = []
	includes = []
	deps = []
	c_args = []
	link_args = []
	language = 'c'

	subdir(dir)

	# basename is useful later.

	_path = dir.split('/')
	basename = _path[_path.length()-1]

	# A convoluted way to remove last component of the path.

	_dirpath = '@' + dir
	_dirpath = _dirpath.strip('-_abcdefghijklmnopqrstuvwxyz0123456789')
	_dirpath = _dirpath.strip('/')
	_dirpath = _dirpath.strip('@')

	# Extra linker flags

	# TODO: let meson do this on install instead, so that disassembly works
	if CONFIG_STRIP_BINARIES
		link_args += [ '-s' ]
	endif

	# Init binaries need to always be linked statically.
	static_build = init.contains(dir)

	# Add the corresponding standard libraries to dependencies.

	deps += [ 'c' ]

	if language == 'cpp'
		deps += 'cpp'
	endif

	# Binaries in the 'drv' subdirectory link libdrv by default.

	if dir.split('/')[0] == 'drv'
		deps += [ 'drv' ]
	endif

	# Convert strings to dependency objects

	_deps = []
	foreach s : deps
		_deps += get_variable((static_build ? 'static_lib' : 'lib') + s)
	endforeach

	# Build executable

	if src.length() > 0
		mapfile = meson.current_build_dir() / dir + '.map'
		# We want linker to generate link map for debugging.
		_ldargs = link_args + [ '-Wl,-Map,' + mapfile ]
		if static_build
			_ldargs += [ '-static' ]
		endif

		_bin = executable(join_paths(dir, basename), src,
			include_directories: includes,
			dependencies: _deps,
			objects: startfiles,
			c_args: c_args,
			link_args: _ldargs,
			implicit_include_directories: false,
			install: true,
			install_dir: _dirpath,
		)

		if disassemble
			custom_target(dir + '.disasm',
				command: [ objdump, '-S', '@INPUT@' ],
				input: _bin,
				output: basename + '.disasm',
				capture: true,
				install: true,
				install_dir: join_paths('debug', _dirpath),
			)
		endif
	endif

	# Build test executable, if any

	if test_src.length() > 0
		mapdir = 'test' / _dirpath
		mapfile = meson.current_build_dir() / mapdir / 'test-' + basename + '.map'
		_ldargs = link_args + [ '-Wl,-Map,' + mapfile ]

		# Make sure the directory exists before the linker runs.
		_mapdir_target = custom_target(mapdir.underscorify(),
			install: false,
			output: mapdir.underscorify(),
			command: [ 'mkdir', '-p', meson.current_build_dir() / mapdir ],
		)

		_bin = executable(join_paths(dir, 'test-' + basename), test_src,
			_mapdir_target,
			include_directories: includes,
			dependencies: [ _deps, static_libpcut ],
			objects: startfiles,
			c_args: c_args,
			link_args: _ldargs,
			implicit_include_directories: false,
			install: true,
			install_dir: join_paths('test', _dirpath),
		)

		if disassemble
			custom_target(join_paths(dir, 'test-' + basename + '.disasm'),
				command: [ objdump, '-S', '@INPUT@' ],
				input: _bin,
				output: 'test-' + basename + '.disasm',
				capture: true,
				install: true,
				install_dir: join_paths('debug', 'test', _dirpath),
			)
		endif
	endif
endforeach
