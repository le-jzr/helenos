
# Redefined in libarch, if necessary.
# Used in a few places where we generate custom assembly.
as_prolog = ''

# List of test sources
testfiles = []

# libc is special
subdir('c')

# TODO: libposix

# IMPORTANT: Dependencies must be listed before libs that depend on them.
libs = [
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

# For each of the subdirectories in the above list, we create a dependency
# object, which is then assigned to a variable name `lib{name}`.
# E.g. to use library in subdirectory `nic` from another library or binary,
# you would use variable `libnic` as a dependency.

foreach l : libs
	# Variables that might be defined in library meson files:

	# List of source files.
	# To set it correctly, use files('list.c', 'of.c', 'files.c')
	# Often this is the only variable that needs to be set.
	src = files()

	# Public include directories. These are used by everyone who depends
	# on this library.
	# Use include_directories('include_dir1', 'include_dir2') to set this.
	# By default, the 'include' subdirectory is used and the variable may
	# be omitted.
	includes = include_directories()

	# Private include directories.
	# Unnecessary if you always include private headers using relative
	# paths, but may be useful if you need to override system headers.
	private_includes = []

	# List of dependency _objects_.
	# Use e.g. deps = [ libnic, libuntar ]
	# Don't just use string names here.
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

	# Set soname.
	soname = 'lib' + l + '.so.' + version.split('.')[0]
	# TODO: set compiler argument
	# TODO: symlink
	# XXX: may be unnecessary with meson

	# Add `include` subdirectory to include dirs if it exists.
	incdir = join_paths(l, 'include')
	if run_command('[', '-d', incdir, ']').returncode() == 0
		includes = [ includes, include_directories(incdir) ]
	endif

	# Pretty much everything depends on libc.
	deps += 'c'

	if language == 'cpp'
		deps += 'cpp'
	endif

	link_args = [ '-Wl,--no-undefined,--no-allow-shlib-undefined' ]
	# FIXME link_args += [ -Wl,-soname,$(LSONAME) ]
	# XXX may be unnecessary

	# Convert strings to dependency objects
	_deps = []
	_static_deps = []
	foreach s : deps
		_deps += get_variable('deplib' + s)
		_static_deps += get_variable('static_lib' + s)
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
			install_dir: join_paths('uspace', 'lib', l),
		)

		if get_option('default_library') == 'static'
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
				install_dir: join_paths('uspace', 'lib', l),
			)
		endif

		_dep = declare_dependency(
			link_with: _lib,
			# TODO: Always use `include` subdirectory for public headers,
			#       and ditch the variable.
			include_directories: includes,
			dependencies: _deps,
		)

		if get_option('default_library') == 'static'
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

	_deplib = _dep

	if not allow_shared
		_dep = _static_dep
	endif

	set_variable('lib' + l, _dep)
	set_variable('deplib' + l, _deplib)
	set_variable('static_lib' + l, _static_dep)
endforeach

# Test binaries
foreach test : testfiles
	testname = test['name']
	libname = testname.split('-')[0]

	executable('test-lib' + testname, test.get('src'),
		include_directories: test.get('includes'),
		dependencies: [ get_variable('lib' + libname), libpcut ],
		objects: startfiles,
		install: true,
		install_dir: join_paths('uspace', 'lib', libname),
	)
endforeach
