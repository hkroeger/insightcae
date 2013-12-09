
USERSHAREDDIR=$HOME/.insight/share
GLOBALSHAREDDIR=/usr/share/insight

for cfgd in $USERSHAREDDIR $GLOBALSHAREDDIR; do # in that order!
 if [ -d $cfgd/python ]; then
  export PYTHONPATH=$cfgd/python:$PYTHONPATH
 fi
done

