always_static = (get_option('default_library') == 'static')

# Redefined in libarch, if necessary.
# Used in a few places where we generate custom assembly.
as_prolog = ''

# IMPORTANT: Dependencies must be listed before libs that depend on them.
libs = [
	'c',

	'block',
	'clui',
	'compress',
	'cpp',
	'crypto',
	'dltest',
	'fdisk',
	'fmtutil',
	'fs',
	'graph',
	'http',
	'label',
	'math',
	'minix',
	'nettl',
	'pcm',
	'pcut',
	'posix',
	'scsi',
	'sif',
	'softrend',
	'trackmod',
	'untar',
	'uri',

	'bithenge',
	'draw',
	'drv',
	'ext4',
	'gui',
	'hound',
	'nic',
	'usb',
	'usbdev',
	'usbhid',
	'usbhost',
	'usbvirt',
	'virtio',

	'ieee80211',
]

# Generated list of include directory paths
include_paths = []

# Generated list of test sources
testfiles = []

foreach l : libs
	# Variables that might be defined in library meson files:

	# List of source files.
	# To set it correctly, use files('list.c', 'of.c', 'files.c')
	# Often this is the only variable that needs to be set.
	src = files()

	# Public include directories. These are used by everyone who depends
	# on this library.
	# Use include_directories('include_dir1', 'include_dir2') to set this.
	# If the variable is omitted, it defaults to 'include' subdirectory
	# if it exists, or the source directory otherwise.
	# If the library has no headers, you can use
	# includes += include_directories()
	includes = []

	# Private include directories.
	# Unnecessary if you always include private headers using relative
	# paths, but may be useful if you need to override system headers.
	private_includes = []

	# List of dependency names.
	# E.g. to use libnic and libuntar use
	# `deps = [ 'nic', 'untar' ]`
	deps = []

	# Extra arguments for the C compiler.
	# Don't use for arguments involving file paths.
	c_args = []

	# Shared object version of the library.
	version = '0.0'

	# Sources of unit tests.
	# Automatically get compiled into a test binary,
	# but not into the library itself.
	test_src = []

	# Language of the library.
	# Currently supported options are 'c' and 'cpp', with 'c' being default.
	language = 'c'

	# Whether the library can be dynamically linked.
	# Eventually, all libraries will be shared by default and this will go away.
	allow_shared = false

	subdir(l)

	# Add `include` or `.` subdirectory to include dirs if needed.
	if includes.length() == 0
		incdir = join_paths(l, 'include')
		if run_command('[', '-d', incdir, ']').returncode() == 0
			includes += include_directories(incdir)
			install_subdir(l / 'include', install_dir: 'include' / 'lib' + l, strip_directory: true)
		else
			includes += include_directories(l)
		endif
	endif

	# Pretty much everything depends on libc.
	if l != 'c'
		deps += 'c'
	endif

	if language == 'cpp' and l != 'cpp'
		deps += 'cpp'
	endif

	mapfile = meson.current_build_dir() / 'lib' + l + '.map'

	link_args = [ '-Wl,--no-undefined,--no-allow-shlib-undefined' ]
	# We want linker to generate link map for debugging.
	link_args += [ '-Wl,-Map,' + mapfile ]

	# Convert strings to dependency objects
	_deps = []
	_static_deps = []
	foreach s : deps
		_deps += get_variable('lib' + s).get('shared')
		_static_deps += get_variable('lib' + s).get('static')
	endforeach

	if src.length() > 0
		_lib = library(l, src,
			# TODO: Include private headers using #include "quoted",
			#       and get rid of private_includes.
			include_directories: [ private_includes, includes ],
			dependencies: _deps,
			c_args: c_args,
			link_args: link_args,
			version: version,
			install: true,
			install_dir: 'lib',
		)

		if disassemble and not always_static
			custom_target('lib' + l + '.disasm',
				command: [ objdump, '-S', '@INPUT@' ],
				input: _lib,
				output: 'lib' + l + '.disasm',
				capture: true,
				install: true,
				install_dir: join_paths('debug', 'lib'),
			)
		endif

		if always_static
			_static_lib = _lib
		else
			_static_lib = static_library(l, src,
				# TODO: Include private headers using #include "quoted",
				#       and get rid of private_includes.
				include_directories: [ private_includes, includes ],
				dependencies: _static_deps,
				c_args: c_args,
				link_args: link_args,
				install: true,
				install_dir: 'lib',
			)
		endif

		_dep = declare_dependency(
			link_with: _lib,
			# TODO: Always use `include` subdirectory for public headers,
			#       and ditch the variable.
			include_directories: includes,
			dependencies: _deps,
		)

		if always_static
			_static_dep = _dep
		else
			_static_dep = declare_dependency(
				link_with: _static_lib,
				# TODO: Always use `include` subdirectory for public headers,
				#       and ditch the variable.
				include_directories: includes,
				dependencies: _static_deps,
			)
		endif
	else
		# Header-only library.
		_dep = declare_dependency(
			include_directories: includes,
			dependencies: _deps,
		)
		_static_dep = declare_dependency(
			include_directories: includes,
			dependencies: _static_deps,
		)
	endif

	if test_src.length() > 0
		testfiles += [ {
			'name': l,
			'src': test_src,
			'includes': [ private_includes, includes ],
		} ]
	endif

	_shared = _dep

	if not allow_shared
		_dep = _static_dep
	endif

	set_variable('lib' + l, {
		'any': _dep,
		'shared': _shared,
		'static': _static_dep,
	})
endforeach

# Test binaries
foreach test : testfiles
	_testname = test['name']
	_libname = _testname.split('-')[0]
	_ldargs = []
	_test_binname = 'test-lib' + _testname

	if link_map
		# We want linker to generate link map for debugging.
		_ldargs += [ '-Wl,-Map,' + meson.current_build_dir() / _test_binname + '.map' ]
	endif

	_bin = executable(_test_binname, test.get('src'),
		include_directories: test.get('includes'),
		dependencies: [ get_variable('lib' + _libname).get('any'), libpcut.get('any') ],
		objects: startfiles,
		install: true,
		install_dir: 'test' / 'lib',
		link_args: _ldargs,
	)

	if disassemble
		custom_target(_test_binname + '.disasm',
			command: [ objdump, '-S', '@INPUT@' ],
			input: _bin,
			output: _test_binname + '.disasm',
			capture: true,
			install: true,
			install_dir: 'debug' / 'test' / 'lib',
		)
	endif
endforeach
