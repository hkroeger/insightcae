#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
SERVER=localhost
META=meta.foam
SUBDIR=""

while getopts "h?ms:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage $0 [-m] [<meta file name>]"
        echo "-m <file name>: meta file name"
        echo "-s <sub dir>: case directory is subdirectory in remote dir"
        exit 0
        ;;
    m)  META=$OPTARG
        ;;
    s)  SUBDIR=$OPTARG
        ;;
    esac
done
shift $((OPTIND-1))

if [ -e "$1" ]; then
 META=$1
fi

if [ -e $META ]; then

 cd $(dirname $META)
 META=$(basename $META)

 read SERVER DIR << EOF
$(cat $META|tr ':' ' ')
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

read LABEL PORT << EOF
$(ssh $SERVER isPVFindPort.sh|tail -n 1)
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

ssh -L$LOCALPORT:127.0.0.1:$PORT $SERVER "cd $DIR ; pvserver --use-offscreen-rendering --server-port=$PORT" &
sleep 1

paraview --server-url=cs://127.0.0.1:$LOCALPORT --data=$DIR/system/controlDict
