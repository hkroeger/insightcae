#!/bin/bash

SERVER=titanp

read LABEL PORT << EOF
$(ssh $SERVER /home/hk354/bin/isPVFindPort.sh|tail -n 1)
EOF

if [ "$LABEL" != "PORT" ]; then
 echo expected "PORT", got "$LABEL"
fi

ssh -L$PORT:localhost:$PORT titanp pvserver --server-port=$PORT &
sleep 1

paraview --server-url=cs://localhost:$PORT
