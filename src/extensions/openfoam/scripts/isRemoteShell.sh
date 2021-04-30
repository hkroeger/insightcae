#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
SERVER=localhost

if [ -e meta.foam ]; then
read SERVER DIR << EOF
$(python3 -c "from Insight.toolkit import *; rec=RemoteExecutionConfig('.', 'meta.foam'); print(rec.server(), rec.remoteDir())")
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

if tty -s; then
 ssh $SERVER -t "cd $DIR; bash -l"
else
 exec mate-terminal -e "ssh $SERVER -t \"cd $DIR; bash -l\""
fi

#exec mate-terminal -e "ssh $SERVER -t \"cd $DIR; bash -l\""

