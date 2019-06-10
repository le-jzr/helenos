#!/bin/sh

# Install generated map files into the 'debug' subdirectory.

cd ${MESON_BUILD_ROOT}/uspace
find -name '*.map' | xargs -I'@' install -C -D -m644 -T '@' "${DESTDIR}/${MESON_INSTALL_PREFIX}/debug/"'@'
cd -

# Install library headers that are mixed in with source files (no separate 'include' subdir).
# The properly separated headers are installed by the meson script.

cd ${MESON_SOURCE_ROOT}/uspace/lib

incdir="${DESTDIR}/${MESON_INSTALL_PREFIX}/include"

for lib in *; do
	if [ -d ${lib} -a ! -d ${lib}/include ]; then
		cd ${lib} || continue
		mkdir -p "${incdir}/lib${lib}"
		find -name '*.h' -a '!' -path './doc/*' | xargs -I'@' install -C -D -m644 -T '@' "${incdir}/lib${lib}/"'@'
		cd -
	fi
done

# Add some helpful importable files with variables.

configdir="${DESTDIR}/${MESON_INSTALL_PREFIX}/config"

sed 's:$(HELENOS_EXPORT_ROOT):${HELENOS_EXPORT_ROOT}:g' < "${configdir}/config.mk" > "${configdir}/config.sh"
cp "${MESON_SOURCE_ROOT}/Makefile.common" "${configdir}"
cp "${MESON_SOURCE_ROOT}/Makefile.config" "${configdir}"
