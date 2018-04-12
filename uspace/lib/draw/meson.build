_libname = 'draw'
_deps = [ libc, libsoftrend , libcompress ]
_extra_cflags = [ ]
_exports = include_directories('.', 'include')
_includes = _exports
_src = files(
	'codec/tga.c',
	'codec/tga.gz.c',
	'codec/webp.c',
	'cursor/embedded.c',
	'font/embedded.c',
	'font/bitmap_backend.c',
	'font/pcf.c',
	'gfx/font-8x16.c',
	'gfx/cursor-11x18.c',
	'drawctx.c',
	'cursor.c',
	'font.c',
	'path.c',
	'source.c',
	'surface.c',
)

libdraw = declare_dependency(
	link_with: library(_libname, _src,
		include_directories: _includes,
		dependencies: _deps,
		c_args: _extra_cflags,
	),
	include_directories: _exports,
	dependencies: _deps,
)
