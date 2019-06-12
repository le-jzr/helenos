#!/bin/sh

(
	# Install generated map files into the 'debug' subdirectory.

	echo "######## Installing library map files ########"

	# TODO: only install when enabled

	cd ${MESON_BUILD_ROOT}/uspace
	find -name '*.map' -a -path './lib/*' | sed 's:^\./::' | xargs --verbose -I'@' install -C -D -m644 -T '@' "${MESON_INSTALL_DESTDIR_PREFIX}debug/"'@'

	# Install library headers that are mixed in with source files (no separate 'include' subdir).
	# The properly separated headers are installed by the meson script.

	echo "######## Installing headers ########"
	cd ${MESON_SOURCE_ROOT}/uspace

	# TODO: only install when enabled

	incdir="${MESON_INSTALL_DESTDIR_PREFIX}include"

	for libdir in lib/*; do
		if [ -d ${libdir} -a ! -d ${libdir}/include ]; then
			find ${libdir} -maxdepth 1 -name '*.h' -a '!' -path ${libdir}'/doc/*' | sed 's:^lib/::' | xargs --verbose -I'@' install -C -D -m644 -T 'lib/@' ${incdir}'/lib@'
		fi
	done

	# Due to certain quirks of our build, executables need to be built with a different name than what they are installed with.
	# Meson doesn't support renaming installed files (at least not as of mid-2019) so we do it here manually.

	echo "######## Installing executables ########"

	cd ${MESON_BUILD_ROOT}/uspace

	find -name 'install@*' |
		sed -e 'h; s:^.*/install@:@DESTDIR@:; s:\$:/:g; x; G; s:\s: :g' -e "s:@DESTDIR@:${MESON_INSTALL_DESTDIR_PREFIX}:g" |
		xargs -L1 --verbose install -C -D -m755 -T

) > ${MESON_BUILD_ROOT}/install_custom.log 2>&1
