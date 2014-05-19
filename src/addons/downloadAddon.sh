#!/bin/bash
# This script downloads a packed git repository and clones it into the addon subdir

URL=$1
TEMPDIR=$(mktemp -d)
SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

(
 cd $TEMPDIR
 wget "$1" -O archive.tgz
 tar xzf archive.tgz
 SD=`find . -iname "*.git"`
 if [ ! -d "$SD" ]; then
  echo "Unexpected archive content!"
  rm -rf $TEMPDIR
  exit -1
 fi

 RN=${SD%.git}
 if [ -d "$SRCDIR/$RN" ]; then
  echo "Addon is present, performing update!"
  cd "$SRCDIR/$RN"
  git pull "$TEMPDIR/$SD"
  exit 0
 else
  echo "Addon is not yet present, cloning new copy!"
  cd $SRCDIR
  git clone "$TEMPDIR/$SD"
  echo "$URL" > $RN/url.txt
 fi
 rm -rf "$TEMPDIR"
)
