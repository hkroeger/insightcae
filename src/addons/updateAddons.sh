#!/bin/bash

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SRCDIR
for i in */url.txt; do
 echo "Updating from $i..."
 $SRCDIR/downloadAddon.sh `cat $i`
done
