
if h_arch == 'ia64'
	# FIXME
	link_args += ['-T', files('elf64_ia64_loader.x')]
else
	link_args += '-Wl,--Ttext-segment=0x70000000'
endif

src = files('main.c')
