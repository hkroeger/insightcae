#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
SERVER=localhost
META=meta.foam
STATEFILE=""
SUBDIR=""

while getopts "h?m:s:r:t:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage $0 [-m] [<meta file name>]"
        echo "-m <file name>: meta file name"
        echo "-s <sub dir>: case directory is subdirectory in remote dir"
        echo "-r <remote host>: continue to <remote host> behind target host (e.g. a cluster node)"
        echo "-t <state file>: load the specified state file"
        exit 0
        ;;
    m)  META=$OPTARG
        ;;
    s)  SUBDIR=$OPTARG
        ;;
    r)  REMHOST=$OPTARG
        ;;
    t)  STATEFILE=$OPTARG
        ;;
    esac
done
shift $((OPTIND-1))

if [ -e "$1" ]; then
 META=$1
fi

function basenameOrNothing() {
  P=`pwd`; if [ "$P" != "/" ]; then basename ${P}$1; fi
}

CASENAME=""
if [ -e $META ]; then

 cd $(dirname $META)
 CASENAME=$(cd ../.. && basenameOrNothing _)$(cd .. && basenameOrNothing _)$(basenameOrNothing)

 META=$(basename $META)

 read SERVER DIR << EOF
$(python3 -c "from Insight.toolkit import *; rec=RemoteExecutionConfig('.', '$META'); print(rec.hostName(), rec.remoteDir())")
EOF

 if [ "$SUBDIR" ]; then
  DIR=$DIR/$SUBDIR
 fi

 echo "connecting to $DIR on $SERVER (read from $META)"

else

 echo "Remote exec config file $META not found!"
 exit -1

fi

#OBJ=${@:$OPTIND:1}
#TOOL=${@:$OPTIND+1:1}
#RES=${@:$OPTIND+2:1}

SSHTARGET=$SERVER
if [ "$REMHOST" ]; then
 SSHTARGET="-J $SERVER $REMHOST"
fi

read LABEL PORT << EOF
$(ssh $SSHTARGET "bash -lc isPVFindPort.sh"|tail -n 1)
EOF
if [ "$LABEL" != "PORT" ]; then
 echo While determining remote port: expected "PORT", got "$LABEL"
fi

read LABEL LOCALPORT << EOF
$(isPVFindPort.sh|tail -n 1)
EOF
if [ "$LABEL" != "PORT" ]; then
 echo While determining local port: expected "PORT", got "$LABEL"
fi

LOG=$(mktemp --tmpdir log_${CASENAME}_XXXX.txt)

if [ -n "$STATEFILE" ]; then
 LOADSCRIPT=$(mktemp --tmpdir loadscript_${CASENAME}_XXXX.py)
 cat > $LOADSCRIPT << EOF
import paraview.simple
paraview.simple.LoadState("$STATEFILE",
                       LoadStateDataFileOptions='Search files under specified directory',
                       DataDirectory='$DIR/system' )
EOF
fi

function cleanup() {
 kill $PVSERVERPID
 rm -vf $LOG $LOADSCRIPT
}

trap 'cleanup' SIGINT

echo "Launching PV server..."
ssh -L$LOCALPORT:127.0.0.1:$PORT $SSHTARGET -t "bash -lc \"cd $DIR ; pvserver-offscreen --use-offscreen-rendering --server-port=$PORT\"" > $LOG 2>&1 &
PVSERVERPID=$!

#tail -f -n+0 $LOG | grep -qe "Accepting connection(s)"
bash -c "echo \"\$\$\"; exec tail -f -n+0 $LOG" | {
  IFS= read pid
  grep -qe "Accepting connection(s)"
  kill "$pid"
}
echo "PV server is up"

echo "Launching PV client..."
if [ -n "$STATEFILE" ]; then
 LOADOPT="--state=$LOADSCRIPT"
else
 LOADOPT="--data=$DIR/system/controlDict"
fi
paraview --title "$CASENAME" --server-url=cs://127.0.0.1:$LOCALPORT $LOADOPT

cleanup
