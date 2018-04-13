
w_flags = [
	'-Wall',
	'-Wextra',
	'-Werror',
]

w_always_error = [
	'implicit-function-declaration',
]

w_enable = [
	'missing-prototypes',
	'write-strings',
]

w_disable = [
	'missing-field-initializers',
	'unused-parameter',
	'typedef-redefinition',
	'clobbered',
]

foreach f : w_always_error
	if meson.get_compiler('c').has_argument('-W' + f)
		w_flags += '-Werror-' + f
	endif
endforeach

foreach f : w_enable
	if meson.get_compiler('c').has_argument('-W' + f)
		w_flags += '-W' + f
	endif
endforeach

foreach f : w_disable
	if meson.get_compiler('c').has_argument('-W' + f)
		w_flags += '-Wno-' + f
	endif
endforeach

add_project_arguments(w_flags, language : [ 'c', 'cpp' ])
