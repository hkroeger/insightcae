#!/bin/bash

export PATH=$PATH:/mxe/usr/bin

cd /mxe

if [ ! -d /mxe/.git ]; then
 export GIT_SSL_NO_VERIFY=1
 git init .; git remote add origin ssh://gogs@rostock.silentdynamics.de:222/silentdynamics/mxe.git; git fetch; git checkout origin/master -ft
 wget -q -O- https://rostock.silentdynamics.de/seafile/f/7d98ebcb06674af689d0/?dl=1 | tar xz
else
 git pull
fi

make -j EXCLUDE_PKGS='librsvg db libsigrok check cryptopp gc gdal gdb gdk-pixbuf ghostscript libsoup upx wxwidgets pcl' MXE_TARGETS=i686-w64-mingw32.shared MXE_PLUGIN_DIRS='plugins/boost_1_66_0 plugins/gcc10'
make -j EXCLUDE_PKGS='check cryptopp gdal ghostscript gst-plugins-bad imagemagick libsigrok ocaml% qtifw upx' MXE_TARGETS=i686-w64-mingw32.static MXE_PLUGIN_DIRS="plugins/gcc10 plugins/boost_1_66_0" nsis
