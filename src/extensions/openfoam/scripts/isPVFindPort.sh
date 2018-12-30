#!/bin/bash


port=11111
quit=0

while [ "$quit" -ne 1 ]; do
  netstat -an | grep $port >> /dev/null
  if [ $? -gt 0 ]; then
    quit=1
  else
    port=`expr $port + 1`
  fi
done

echo PORT $port
