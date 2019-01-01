#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
SERVER=localhost

if [ -e meta.foam ]; then
read SERVER DIR << EOF
$(cat meta.foam|tr ':' ' ')
EOF
fi

while getopts "h?s" opt; do
    case "$opt" in
    h|\?)
        echo "-s: server address"
        exit 0
        ;;
    s)  SERVER=s
        ;;
    esac
done

#OBJ=${@:$OPTIND:1}
#TOOL=${@:$OPTIND+1:1}
#RES=${@:$OPTIND+2:1}

read LABEL PORT << EOF
$(ssh $SERVER isPVFindPort.sh|tail -n 1)
EOF

if [ "$LABEL" != "PORT" ]; then
 echo expected "PORT", got "$LABEL"
fi

ssh -L$PORT:localhost:$PORT $SERVER "cd $DIR ; pvserver --server-port=$PORT" &
sleep 1

paraview --server-url=cs://localhost:$PORT --data=$DIR/system/controlDict
