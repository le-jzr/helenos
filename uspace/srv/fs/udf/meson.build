_name = 'udf'
_deps = [ libc, libblock , libfs ]
_extra_cflags = [ ]
_includes = include_directories('.')
_src = files(
	'udf.c',
	'udf_volume.c',
	'udf_ops.c',
	'udf_osta.c',
	'udf_cksum.c',
	'udf_file.c',
	'udf_idx.c',
)

udf = executable(_name, _src,
	include_directories: _includes,
	dependencies: _deps,
	c_args: _extra_cflags,
)
