_name = 'bdsh'
_deps = [ libc, libclui, libfmtutil ]
_extra_cflags = []
_includes = include_directories('cmds', 'cmds/builtins', 'cmds/modules')
_src = files(
	'cmds/modules/help/help.c',
	'cmds/modules/mkdir/mkdir.c',
	'cmds/modules/mkfile/mkfile.c',
	'cmds/modules/rm/rm.c',
	'cmds/modules/cat/cat.c',
	'cmds/modules/touch/touch.c',
	'cmds/modules/ls/ls.c',
	'cmds/modules/pwd/pwd.c',
	'cmds/modules/sleep/sleep.c',
	'cmds/modules/cp/cp.c',
	'cmds/modules/mv/mv.c',
	'cmds/modules/printf/printf.c',
	'cmds/modules/echo/echo.c',
	'cmds/modules/mount/mount.c',
	'cmds/modules/unmount/unmount.c',
	'cmds/modules/kcon/kcon.c',
	'cmds/modules/cmp/cmp.c',
	'cmds/builtins/batch/batch.c',
	'cmds/builtins/exit/exit.c',
	'cmds/builtins/cd/cd.c',
	'cmds/mod_cmds.c',
	'cmds/builtin_cmds.c',
	'compl.c',
	'errors.c',
	'input.c',
	'util.c',
	'exec.c',
	'scli.c',
	'tok.c',
)

bdsh = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)

_test_src = files(
	'tok.c',
	'test/toktest.c',
)

test_bdsh = executable('test_bdsh', _test_src,
	include_directories: _includes,
	dependencies: [ _deps, libpcut ],
)
