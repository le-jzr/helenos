_name = 'tcp'
_deps = [ libc, libnettl ]
_extra_cflags = [ ]
_includes = include_directories('.')
_common_src = files(
	'conn.c',
	'inet.c',
	'iqueue.c',
	'ncsim.c',
	'pdu.c',
	'rqueue.c',
	'segment.c',
	'seq_no.c',
	'test.c',
	'tqueue.c',
	'ucall.c',
)
_src = files(
	'service.c',
	'tcp.c',
)

tcp = executable(_name, _common_src, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)

_test_src = files(
	'test/conn.c',
	'test/iqueue.c',
	'test/main.c',
	'test/pdu.c',
	'test/rqueue.c',
	'test/segment.c',
	'test/seq_no.c',
	'test/tqueue.c',
	'test/ucall.c',
)

test_tcp = executable('test_tcp', _common_src, _test_src,
	include_directories: _includes,
	dependencies: [ _deps, libpcut ],
)
