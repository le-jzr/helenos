#!/bin/sh

# Install generated map files into the 'debug' subdirectory.
cd ${MESON_BUILD_ROOT}/uspace
find -name '*.map' | xargs -I'@' install -C -D -m644 -T '@' "${DESTDIR}/${MESON_INSTALL_PREFIX}/debug/"'@'
