
grub_config = []
grub_config += 'set default=0'
grub_config += 'set timeout=10'
grub_config += ''

if config_grub_arch == 'pc'
	grub_config += 'insmod vbe'
	grub_config += 'insmod vga'
endif
if config_grub_arch == 'efi'
	grub_config += 'insmod efi_gop'
	grub_config += 'insmod efi_uga'
endif

grub_config += ''
grub_config += '''menuentry 'HelenOS ''' + h_release + ''' ' --class helenos --class os {'''

foreach module: modules
	grub_config += '''	echo 'Loading $$module' '''
	if module == 'kernel.bin'
		grub_config += '	' + multiboot_cmd + ' /boot/' + module
	else
		grub_config += '	' + module_cmd + ' /boot/' + module + ' /boot/' + module
	endif
endforeach

grub_config += '}'