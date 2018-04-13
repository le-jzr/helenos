_name = 'vlaunch'
_deps = [ libgui ]

_images = files(
	'gfx/helenos.tga',
)

_images_zip = custom_target('app_vlaunch_images.zip',
	input : _images,
	output : [ 'images.zip' ],
	command : [ mkarray, '@OUTDIR@', 'images', 'image', as_prolog, '.data', '@INPUT@' ],
)
_imgs_s = custom_target('app_vlaunch_images.s',
	input : _images_zip,
	output : [ 'images.s' ],
	command : [ unzip, '-p', '@INPUT@', 'images.s' ],
	capture : true,
)
_imgs_h = custom_target('app_vlaunch_images.h',
	input : _images_zip,
	output : [ 'images.h' ],
	command : [ unzip, '-p', '@INPUT@', 'images.h' ],
	capture : true,
)
_imgs_desc_c = custom_target('app_vlaunch_images_desc.c',
	input : _images_zip,
	output : [ 'images_desc.c' ],
	command : [ unzip, '-p', '@INPUT@', 'images_desc.c' ],
	capture : true,
)

_src = files(
	'vlaunch.c',
)
_src += [ _imgs_s, _imgs_h, _imgs_desc_c ]

vlaunch = executable(_name, _src, dependencies: _deps)
