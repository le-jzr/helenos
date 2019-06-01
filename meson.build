# TODO: Use vcs_tag() to generate version string
# TODO: jobfile
# TODO: lto
# TODO: -Werror on CONFIG_DEBUG
# TODO: OPTIMIZATION flag
# TODO: -D__$(ENDIANESS)__ in cross
# TODO: CONFIG_BUILD_SHARED_LIBRARIES, CONFIG_USE_SHARED_LIBRARIES
# TODO: fix clang build
# TODO: fix xcw

project(
    'HelenOS',
    [ 'c', 'cpp' ],
    default_options : ['buildtype=plain', 'c_std=gnu11', 'cpp_std=c++17', 'warning_level=3', 'werror=true', 'b_staticpic=false', 'default_library=shared' ],
    version : '0.9.1-git',
)

if get_option('default_library') == 'both'
	error('You must use either shared or static for default_library.')
endif

# Output compiler flags for use by third-party builds.
# NOTE: See $srcroot/meson/cross/$arch for architecture-specific compiler flags.

message('Cross c_args:')
message(meson.get_cross_property('c_args'))
message('Cross cpp_args:')
message(meson.get_cross_property('cpp_args'))
message('Cross c_link_args:')
message(meson.get_cross_property('c_link_args'))
message('Cross cpp_link_args:')
message(meson.get_cross_property('cpp_link_args'))

cc = meson.get_compiler('c')

add_project_link_arguments(
	cc.get_supported_link_arguments([
		'-Wl,--gc-sections',
		'-Wl,--warn-common',
	]),
	'-Wl,--fatal-warnings',
	language : [ 'c', 'cpp' ],
)

add_project_arguments(
	'-imacros', join_paths(meson.source_root(), 'config.h'),
	language : [ 'c' ],
)

find = find_program('find')
unzip = find_program('unzip')
mkarray = find_program('tools/mkarray_for_meson.sh')
#config_header = files('config.h')

CONFIG_RDFMT = get_option('rdfmt')
CONFIG_UBSAN = get_option('ubsan')
# TODO
CONFIG_BUILD_SHARED_LIBS = false
CONFIG_DEBUG = true
CONFIG_STRIP_BINARIES = false
CONFIG_LINE_DEBUG = false
CONFIG_RTLD = true

h_arch = meson.get_cross_property('h_arch')

# FIXME
h_release = meson.project_version()
h_copyright = 'Copyright (c) 2001-2019 HelenOS project'
h_name = 'Armonia'

# TODO: enable more warnings
# XXX: -fno-builtin-strftime works around seemingly spurious format warning.
# TODO: CONFIG_DEBUG, CONFIG_UBSAN

extra_common_flags = [
	'-O3',
	'-fexec-charset=UTF-8',
	'-finput-charset=UTF-8',

	'-D_HELENOS_SOURCE',
	'-Wa,--fatal-warnings',

	'-Wall',
	'-Wextra',

	'-Werror-implicit-function-declaration',

	'-Wwrite-strings',
	'-Wsystem-headers',
	'-Wunknown-pragmas',

	'-Wno-unused-parameter',

	'-pipe',
	'-ffunction-sections',
	'-fvar-tracking-assignments',
	'-fno-common',

	'-fdebug-prefix-map=' + meson.source_root() + '=.',
]

if CONFIG_LINE_DEBUG
	extra_common_flags += [ '-gdwarf-4', '-g3' ]
endif

extra_cflags = extra_common_flags + [
	'-Wmissing-prototypes',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-command-line-argument',
	'-Wno-unused-parameter',
	'-Wno-typedef-redefinition',
	'-Wno-clobbered',
	'-Wno-nonnull-compare',

	'-fno-builtin-strftime',
]

if CONFIG_UBSAN
	extra_cflags += '-fsanitize=undefined'
endif

extra_cppflags = extra_common_flags + [
	'-fno-exceptions',
	'-frtti',
]

w_flags = {
	'c': extra_cflags,
	'cpp': extra_cppflags,
}

# Process flags. Only sets those that compiler supports.
foreach lang : [ 'c', 'cpp' ]
	_args = meson.get_compiler(lang).get_supported_arguments(w_flags.get(lang))
	add_project_arguments(_args, language : [ lang ])
endforeach

subdir('uspace')
