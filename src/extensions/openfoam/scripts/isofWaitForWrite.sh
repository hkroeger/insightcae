#!/bin/bash

function help() {
        cat << EOM
Usage: $0 [-p] [-c <path>]

  -p         parallel run
  -c <path>  specify case directory, default "."
EOM
        exit 0
}

CASEDIR="."
while getopts "pc:" opt; do
  case "${opt}" in
    p) PARALLEL=1;;
    c) CASEDIR="$OPTARG";;
    *) help;;
  esac
done
shift $((OPTIND-1))

CHECKDIR="$CASEDIR"
if [ -n "$PARALLEL" ]; then
 CHECKDIR="$CHECKDIR/processor0"
fi

if [ ! -d "$CHECKDIR" ]; then
 echo "The directory $CHECKDIR does not exist!"
 exit -1
fi

function lastTimeDir() {
 ls -1 $1 | awk '$1 ~ /^[0-9\.eE+-]*$/{print $1}' | sort -g | tail -1
}

LTIME=$(lastTimeDir "${CHECKDIR}")
echo "Last present time directory: $LTIME"

touch "$CASEDIR/wnow"

echo -n "Waiting for new time directory to be written "
DONE=
while [ ! "$DONE" ]; do
 sleep 1
 echo -n "."
 CURLTIME=$(lastTimeDir "${CHECKDIR}")
# echo $LTIME $CURLTIME
 if [ "$LTIME" != "$CURLTIME" ]; then
  DONE=1
 fi
done
echo ""

echo "Found new time directory: $CURLTIME"

if [ "$PARALLEL" ]; then
 reconstructPar -case "$CASEDIR" -latestTime
fi
