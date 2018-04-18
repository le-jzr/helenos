
# Redefined in libarch, if necessary.
# Used in a few places where we generate custom assembly.
as_prolog = ''

# FIXME: There's a bit of a cyclic dependency between libc and libpcut due to
# the way code (tests) is organized.
libc_includes = include_directories(
	'c/include',
	'c/arch/' + h_arch + '/include',
	'../../abi/include',
)

# XXX: It's important that dependencies are listed before libs that depend on them.
libs = [
	# These two must be first
	'pcut',
	'c',

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

foreach l : libs
	src = []
	includes = []
	deps = []
	c_args = []
	private_includes = []
	version = '0.0'
	test_src = []

	subdir(l)

	# Add `include` subdirectory to include dirs if it exists.
	incdir = join_paths(l, 'include')
	if run_command('[', '-d', incdir, ']').returncode() == 0
		includes = [ includes, include_directories(incdir) ]
	endif

	if is_variable('libc')
		# Pretty much everything depends on libc.
		deps += libc
	endif

	if src.length() > 0
		dep = declare_dependency(
			link_with: library(l, src,
				# TODO: Include private headers using #include "quoted",
				#       and get rid of private_includes.
				include_directories: [ private_includes, includes ],
				dependencies: deps,
				c_args: c_args,
				version: version,
			),
			# TODO: Always use `include` subdirectory for public headers,
			#       and ditch the variable.
			include_directories: includes,
			dependencies: deps,
		)
	else
		# Header-only library.
		dep = declare_dependency(
			include_directories: includes,
			dependencies: deps,
		)
	endif

	if is_variable('libpcut') and test_src.length() > 0
		executable('test_lib' + l, test_src,
			include_directories: [ private_includes, includes ],
			dependencies: [ dep, libpcut ],
		)
	endif

	set_variable('lib' + l, dep)
endforeach
