#this file shall be source'd in ~/.bashrc
SCRPATH=${BASH_SOURCE[0]}

export INSIGHT_BINDIR=$(cd `dirname $SCRPATH`; pwd)
export INSIGHT_INSTDIR=$(cd $INSIGHT_BINDIR; cd ..; pwd)
export INSIGHT_LIBDIR=${INSIGHT_INSTDIR}/lib

export INSIGHT_USERSHAREDDIR=$HOME/.insight/share
export INSIGHT_GLOBALSHAREDDIRS="$INSIGHT_INSTDIR/share/insight /usr/share/insight"

export PATH=$INSIGHT_BINDIR:$PATH
export LD_LIBRARY_PATH=$INSIGHT_LIBDIR:$LD_LIBRARY_PATH

for cfgd in $INSIGHT_USERSHAREDDIR $INSIGHT_GLOBALSHAREDDIRS; do # in that order!
 if [ -d $cfgd/python ]; then
  export PYTHONPATH=$cfgd/python:$PYTHONPATH
 fi
done

