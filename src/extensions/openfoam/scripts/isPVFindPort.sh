#!/bin/bash

forbiddenports="$*"
port=${port:-1024}
quit=0

if grep -qEi "(Microsoft|WSL)" /proc/version &> /dev/null ; then

    while [ "$quit" -ne 1 ]; do

      if ! ( netstat.exe -an 2>&1|grep LISTENING|grep  :$port[^0-9] > /dev/null ) && ! (echo $forbiddenports | grep -w -q $port ); then
        quit=1
      else
        port=`expr $port + 1`
      fi
    done

else

    while [ "$quit" -ne 1 ]; do

      if ! ( netstat -tulpn 2>&1 |grep  :$port[^0-9] > /dev/null ) && ! (echo $forbiddenports | grep -w -q $port ); then
        quit=1
      else
        port=`expr $port + 1`
      fi
    done

fi


echo PORT $port
