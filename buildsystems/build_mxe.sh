#!/bin/bash

export PATH=$PATH:/mxe/usr/bin

cd /mxe

# no /opt/insightcae/python3.6 in path for meson!
make -j MXE_TARGETS=i686-w64-mingw32.shared MXE_PLUGIN_DIRS='plugins/boost_1_66_0' armadillo boost dxflib freetype gcc libgcrypt glib gsl harfbuzz hdf5 libiconv libidn libidn2 vtk oce openssl jpeg qt5 cryptopp poppler libntlm openssl wt tiff libgsasl
make -j MXE_TARGETS=i686-w64-mingw32.static MXE_PLUGIN_DIRS="plugins/boost_1_66_0" nsis
