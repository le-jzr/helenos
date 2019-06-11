link_map = true
disassemble = true

if disassemble
	objdump = find_program('objdump')
endif

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

bin_targets = []

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
		_deps += get_variable('lib' + s).get(static_build ? 'static' : 'any')
	endforeach

	# Build executable

	if src.length() > 0
		bin_targets += {
			'src': src,
			'dirname': _dirpath,
			'basename': basename,
			'install_dir': _dirpath,
			'includes': includes,
			'dependencies': _deps,
			'c_args': c_args,
			'link_args': link_args + (static_build ? [ '-static' ] : []),
		}
	endif

	# Build test executable, if any

	if test_src.length() > 0
		bin_targets += {
			'src': test_src,
			'dirname': _dirpath,
			'basename': 'test-' + basename,
			'install_dir': 'test' / _dirpath,
			'includes': includes,
			'dependencies': [ _deps, libpcut.get('any') ],
			'c_args': c_args,
			'link_args': link_args,
		}
	endif
endforeach

foreach tst : bin_targets
	_dir = tst.get('dirname') / tst.get('basename')
	_ldargs = tst.get('link_args')
	_src = tst.get('src')

	if link_map
		# We want linker to generate link map for debugging.
		_ldargs += [ '-Wl,-Map,' + meson.current_build_dir() / _dir.underscorify() + '.map' ]
	endif

	_bin = executable(_dir.underscorify(), _src,
		include_directories: tst.get('includes'),
		dependencies: tst.get('dependencies'),
		objects: startfiles,
		c_args: tst.get('c_args'),
		link_args: _ldargs,
		implicit_include_directories: false,
		install: false,
		#install_dir: tst.get('install_dir'),
	)

	if disassemble
		custom_target(_dir.underscorify() + '.disasm',
			command: [ objdump, '-S', '@INPUT@' ],
			input: _bin,
			output: _dir.underscorify() + '.disasm',
			capture: true,
			install: false,
			#install_dir: 'debug' / tst.get('install_dir'),
			build_by_default: true,
		)
	endif
endforeach
