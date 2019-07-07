#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

VERSION=6.0.3
DIR=scotch_$VERSION

[ -d $DIR ] || ( wget -q -O- https://gforge.inria.fr/frs/download.php/file/34099/scotch_6.0.3.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 cd src

 #cp Make.inc/Makefile.inc.x86-64_pc_linux2.shlib ./Makefile.inc
 cat > Makefile.inc << EOF
EXE             =
LIB             = .so
OBJ             = .o

MAKE            = make
AR              = gcc
ARFLAGS         = -shared -o
CAT             = cat
CCS             = gcc
CCP             = mpicc
CCD             = gcc
CFLAGS          = -O3 -DCOMMON_FILE_COMPRESS_GZ -DCOMMON_PTHREAD -DCOMMON_RANDOM_FIXED_SEED -DSCOTCH_RENAME -DSCOTCH_PTHREAD -Drestrict=__restrict -DIDXSIZE64 -I/usr/include/openmpi-x86_64
CLIBFLAGS       = -shared -fPIC
LDFLAGS         = -lz -lm -lrt -pthread
CP              = cp
LEX             = flex -Pscotchyy -olex.yy.c
LN              = ln
MKDIR           = mkdir
MV              = mv
RANLIB          = echo
YACC            = bison -pscotchyy -y -b y
EOF

 make -j48
 make ptscotch -j48

 cp -v ../lib/* $INSTALLDIR/lib
 cp -v ../include/* $INSTALLDIR/include
)
