#!/bin/sh

(
	# Install generated map files into the 'debug' subdirectory.

	echo "######## Installing map files ########"

	cd ${MESON_BUILD_ROOT}/uspace
	find -name '*.map' -a -path './*/*' | sed 's:^\./::' | xargs --verbose -I'@' install -C -D -m644 -T '@' "${MESON_INSTALL_DESTDIR_PREFIX}debug/"'@'

	# Install library headers that are mixed in with source files (no separate 'include' subdir).
	# The properly separated headers are installed by the meson script.

	echo "######## Installing headers ########"
	cd ${MESON_SOURCE_ROOT}/uspace

	incdir="${MESON_INSTALL_DESTDIR_PREFIX}include"

	for libdir in lib/*; do
		if [ -d ${libdir} -a ! -d ${libdir}/include ]; then
			find ${libdir} -maxdepth 1 -name '*.h' -a '!' -path ${libdir}'/doc/*' | sed 's:^lib/::' | xargs --verbose -I'@' install -C -D -m644 -T 'lib/@' ${incdir}'/lib@'
		fi
	done

	# Due to certain quirks of our build, executables are built with a different name than what they are installed with.
	# Meson doesn't support renaming installed files (at least not as of mid-2019) so we do it here manually.

	echo "######## Installing executables ########"

	cd ${MESON_SOURCE_ROOT}/uspace

	for subdir in app drv srv; do
		# Get list of Meson subdirectories.
		dirs=`find -name meson.build -a -path ./${subdir}'/*/*' | sed 's/^\.\/\(.*\)\/meson.build$/\1/'`

		for dir in $dirs; do
			underscored=`echo $dir | sed s:[/-]:_:g`
			testname=`dirname $dir`/test-`basename $dir`
			testname_underscored=`echo $testname | sed s:[/-]:_:g`

			if [ -f ${MESON_BUILD_ROOT}/uspace/${underscored} ]; then
				echo | xargs --verbose install -C -D -m755 -T ${MESON_BUILD_ROOT}/uspace/${underscored}        ${MESON_INSTALL_DESTDIR_PREFIX}${dir}
			fi
			if [ -f ${MESON_BUILD_ROOT}/uspace/${underscored}.map ]; then
				echo | xargs --verbose install -C -D -m755 -T ${MESON_BUILD_ROOT}/uspace/${underscored}.map    ${MESON_INSTALL_DESTDIR_PREFIX}debug/${dir}.map
			fi
			if [ -f ${MESON_BUILD_ROOT}/uspace/${underscored}.disasm ]; then
				echo | xargs --verbose install -C -D -m755 -T ${MESON_BUILD_ROOT}/uspace/${underscored}.disasm ${MESON_INSTALL_DESTDIR_PREFIX}debug/${dir}.disasm
			fi

			if [ -f ${MESON_BUILD_ROOT}/uspace/${testname_underscored} ]; then
				echo | xargs --verbose install -C -D -m755 -T ${MESON_BUILD_ROOT}/uspace/${testname_underscored}        ${MESON_INSTALL_DESTDIR_PREFIX}test/${testname}
			fi
			if [ -f ${MESON_BUILD_ROOT}/uspace/${testname_underscored}.map ]; then
				echo | xargs --verbose install -C -D -m755 -T ${MESON_BUILD_ROOT}/uspace/${testname_underscored}.map    ${MESON_INSTALL_DESTDIR_PREFIX}debug/test/${testname}.map
			fi
			if [ -f ${MESON_BUILD_ROOT}/uspace/${testname_underscored}.disasm ]; then
				echo | xargs --verbose install -C -D -m755 -T ${MESON_BUILD_ROOT}/uspace/${testname_underscored}.disasm ${MESON_INSTALL_DESTDIR_PREFIX}debug/test/${testname}.disasm
			fi
		done
	done

) > ${MESON_BUILD_ROOT}/install_custom.log 2>&1
