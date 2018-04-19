project(
    'HelenOS userland',
    [ 'c' ],
    default_options : ['buildtype=plain', 'c_std=gnu99', 'warning_level=3', 'werror=true', 'b_staticpic=false', 'default_library=static' ],
    version : '0.7.1-git',
)

find = find_program('find')
unzip = find_program('unzip')
mkarray = find_program('../tools/mkarray_for_meson.sh')
#config_header = files('../config.h')

add_project_link_arguments('-Wl,--fatal-warnings', '-Wl,--warn-common', language : [ 'c', 'cpp' ])
add_project_arguments('-D_HELENOS_SOURCE', '-Wa,--fatal-warnings', language : [ 'c', 'cpp' ])

h_arch = meson.get_cross_property('h_arch')

# FIXME
h_release = meson.project_version()
h_copyright = 'Copyright (c) 2001-2018 HelenOS project'
h_name = 'Enlightened Zeal'

subdir('meson/warnings')
subdir('lib')
subdir('app')

# TODO: clean up
subdir('srv')
subdir('drv')
