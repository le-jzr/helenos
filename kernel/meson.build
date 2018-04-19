project(
    'HelenOS kernel',
    [ 'c' ],
    default_options : ['buildtype=plain', 'c_std=gnu99', 'warning_level=3', 'werror=true', 'b_staticpic=false', 'default_library=static' ],
    version : '0.7.1-git',
)

subdir(join_paths('arch', h_arch))

## The at-sign
#
# The `atsign` variable holds the ASCII character representing the at-sign
# ('@') used in various $(AS) constructs (e.g. @progbits). On architectures that
# don't use '@' for starting a comment, `atsign` is merely '@'. However, on
# those that do use it for starting a comment (e.g. arm32), `atsign` must be
# defined as the percentile-sign ('%') in the architecture-dependent files.
#
if !is_variable('atsign')
	atsign = '@'
endif

symtab_section = '.section symtab.data, "a", ' + atsign + 'progbits;'

subdir('genarch')
subdir('generic')

includes = include_directories('generic/include', 'genarch/include', 'arch/' + h_arch + '/include', '../abi/include')

defs = [ '-DKERNEL' '-DRELEASE=' + h_release, '-DCOPYRIGHT=' + h_copyright, '-DNAME=' h_name, '-D__' + bits + '_BITS__', '-D__' + endianness + '__' ]


c_args += [ '-Wa,--fatal-warnings' ]
link_args += [ '-Wl,-n,-T,$(LINK),-M,--fatal-warnings,--warn-common' ]

if config_strip_binaries
	link_args += '--strip-all'
endif

c_args += ['-O' + config_optimization, '-ffreestanding', '-nostdlib', '-nostdinc',
	'-fexec-charset=UTF-8', '-finput-charset=UTF-8', '-fno-common',
	'-std=gnu99', '-Wall', '-Wextra', '-Wno-unused-parameter',
	'-pipe', '-Wmissing-prototypes', '-Werror-implicit-function-declaration',
	'-Wwrite-strings' ]

if config_debug
	c_args += '-Werror'
endif

if config_lto
	c_args += '-flto'
endif

if config_line_debug
	c_args += '-g'
endif

# TODO: clang, -Wno-missing-field-initializers -Wno-unused-command-line-argument

#ifeq ($(COMPILER),clang)
#	AS_CFLAGS += -mllvm -asm-macro-max-nesting-depth=1000
#endif

# TODO: -fdebug-prefix-map=$(realpath $(ROOT_PATH))=.

src = [
]