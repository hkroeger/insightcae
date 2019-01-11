#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
SERVER=localhost
META=meta.foam

while getopts "h?m:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage $0 [-m] [<meta file name>]"
        echo "-m <file name>: meta file name"
        exit 0
        ;;
    m)  META=$OPTARG
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

ssh -L$LOCALPORT:localhost:$PORT $SERVER "cd $DIR ; pvserver --use-offscreen-rendering --server-port=$PORT" &
sleep 1

paraview --server-url=cs://localhost:$LOCALPORT --data=$DIR/system/controlDict
