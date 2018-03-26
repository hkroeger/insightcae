#!/bin/bash

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SRCDIR

for i in `find . -mindepth 1 -maxdepth 1 -type d`; do
 (
  cd $i
  if [ -d ".git" ]; then
   echo "Updating $i..."
   git "$@"
  else
   echo "Skipping $i, because it is not a GIT repository."
  fi
 )
done
