# TODO: Use vcs_tag() to generate version string
# TODO: jobfile
# TODO: lto
# TODO: -Werror on CONFIG_DEBUG
# TODO: OPTIMIZATION flag
# TODO: -D__$(ENDIANESS)__ in cross
# TODO: CONFIG_BUILD_SHARED_LIBRARIES, CONFIG_USE_SHARED_LIBRARIES
# TODO: fix clang build
# TODO: fix xcw
# TODO: CONFIG_STRIP_BINARIES
# TODO: export cross flags
# TODO: CONFIG_PCUT_TESTS, CONFIG_PCUT_SELF_TESTS
# TODO: only install static libraries and headers when CONFIG_DEVEL_FILES is enabled

project(
    'HelenOS',
    [ 'c', 'cpp' ],
    default_options : ['buildtype=plain', 'c_std=gnu11', 'cpp_std=c++17', 'warning_level=3', 'werror=true', 'b_staticpic=false', 'default_library=shared', 'prefix=/' ],
    version : '0.9.1-git',
)

meson.add_install_script('install.sh')

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
sed = find_program('sed')
mkarray = find_program('tools/mkarray_for_meson.sh')
dirname = find_program('dirname')
basename = find_program('basename')
which = find_program('which')

autocheck = generator(find_program('tools/autocheck.awk'),
	arguments: [ '@INPUT@' ],
	output: '@PLAINNAME@.check.c',
	capture: true,
)

CONFIG_RDFMT = get_option('rdfmt')
CONFIG_UBSAN = get_option('ubsan')
# TODO
CONFIG_BUILD_SHARED_LIBS = true
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


## Generate config.mk

# Get the directory where the compiler resides.

cc_fullname = run_command(which, meson.get_compiler('c').cmd_array()[0].split(' ')[0], check: true).stdout().strip()
cc_path = run_command(dirname, cc_fullname, check: true).stdout().strip()

message(['Compiler directory is:', cc_path])

export_cppflags = [
	'-isystem', '$(HELENOS_EXPORT_ROOT)/include/posix',
	'-isystem', '$(HELENOS_EXPORT_ROOT)/include/libc',
	'-idirafter', '$(HELENOS_EXPORT_ROOT)/include',
]

export_ldflags = [
	'-nostdlib',
	'-L$(HELENOS_EXPORT_ROOT)/lib',
	'-Wl,--whole-archive',
	'$(HELENOS_EXPORT_ROOT)/lib/libstartfiles.a',
	'-Wl,--no-whole-archive',
]

export_ldlibs = [
	'-Wl,--as-needed',
	'-lposix',
	'-lmath',
	'-lc',
	'-lgcc',
	'-Wl,--no-as-needed',
]

cc_arch = meson.get_cross_property('cc_arch')

conf_data = configuration_data({
	'HELENOS_CROSS_PATH' : cc_path,
	'HELENOS_ARCH' : cc_arch,
	'HELENOS_CFLAGS' : ' '.join(meson.get_cross_property('c_args')),
	'HELENOS_CXXFLAGS' : ' '.join(meson.get_cross_property('cpp_args')),
	'HELENOS_CPPFLAGS' : ' '.join(export_cppflags),
	'HELENOS_LDFLAGS' : ' '.join(export_ldflags),
	'HELENOS_LDLIBS' : ' '.join(export_ldlibs),
	'HELENOS_TARGET' : cc_arch + '-helenos',
})

config_mk = configure_file(
	input: 'config.mk.in',
	output: 'config.mk',
	configuration: conf_data,
	install: true,
	install_dir: 'config',
)

custom_target('config.sh',
	input: config_mk,
	output: 'config.sh',
	command: [ sed, 's:$(HELENOS_EXPORT_ROOT):${HELENOS_EXPORT_ROOT}:g', '@INPUT@' ],
	capture: true,
	install: true,
	install_dir: 'config',
)

install_data('Makefile.common', 'Makefile.config', install_dir: 'config')
