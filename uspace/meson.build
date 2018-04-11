project(
    'HelenOS userland',
    [ 'c' ],
    default_options : ['buildtype=plain'],
    version : '0.7.1-git',
)

add_global_link_arguments('-Wl,--fatal-warnings', language : [ 'c', 'cpp' ])
add_global_arguments('-D_HELENOS_SOURCE', '-Wa,--fatal-warnings', language : [ 'c', 'cpp' ])

libdirs = [
	'lib/c',
]

h_arch = meson.get_cross_property('h_arch')

if meson.get_compiler('c').has_argument('-Wno-missing-field-initializers')
	# TODO
endif

subdir(libdirs)