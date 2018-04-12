project(
    'HelenOS userland',
    [ 'c' ],
    default_options : ['buildtype=plain', 'c_std=gnu11', 'warning_level=3', 'werror=true', 'b_staticpic=false', 'default_library=static' ],
    version : '0.7.1-git',
)

add_global_link_arguments('-Wl,--fatal-warnings', '-nostdlib', '-static', '-Wl,-e,__entry', language : [ 'c', 'cpp' ])
add_global_arguments('-D_HELENOS_SOURCE', '-Wa,--fatal-warnings', language : [ 'c', 'cpp' ])

h_arch = meson.get_cross_property('h_arch')

if meson.get_compiler('c').has_argument('-Wno-missing-field-initializers')
	# TODO
endif

# There's a bit of a cyclic dependency between libc and libpcut due to the
# way code is organized.
libc_includes = include_directories(
	'lib/c/include',
	'lib/c/arch/' + h_arch + '/include',
	'../abi/include',
)
subdir('lib/pcut')
subdir('lib/c')

subdirs = [
	'lib/clui',
	'lib/fmtutil',
	'app/bdsh',
	#'lib/bithenge',
]

foreach d : subdirs
	subdir(d)
endforeach