#this file shall be source'd in ~/.bashrc
SCRPATH=${BASH_SOURCE[0]}

export INSIGHT_BINDIR=$(cd `dirname $SCRPATH`; pwd)
export INSIGHT_CONFIGSCRIPT=${INSIGHT_BINDIR}/$(basename ${SCRPATH})
export INSIGHT_INSTDIR=$(cd $INSIGHT_BINDIR; cd ..; pwd)
# basic library directory
export INSIGHT_LIBDIR=${INSIGHT_INSTDIR}/lib
# all library directories, including pvoffscreen
export INSIGHT_LIBDIRS=$INSIGHT_LIBDIR:${INSIGHT_INSTDIR}/paraview-offscreen/lib

export INSIGHT_USERSHAREDDIR=$HOME/.insight/share
export INSIGHT_GLOBALSHAREDDIRS=$INSIGHT_INSTDIR/share/insight:/usr/share/insight


## check if thirdparty config exists, load if yes
INSIGHT_THIRDPARTY_CONFIGSCRIPT=${INSIGHT_BINDIR}/insight_setthirdpartyenv.sh
if [ -e ${INSIGHT_THIRDPARTY_CONFIGSCRIPT} ]; then
 source ${INSIGHT_THIRDPARTY_CONFIGSCRIPT}
fi


## setup PATH and LD_LIBRARY_PATH
if [ -n "$LD_LIBRARY_PATH" ]; then
 ORG_LDPATH=:${LD_LIBRARY_PATH}
fi
if [ -n "$PATH" ]; then
 ORG_PATH=:${PATH}
fi

[[ ":$PATH:" != *":${INSIGHT_BINDIR}:"* ]] && PATH="${INSIGHT_BINDIR}"
[[ ":$LD_LIBRARY_PATH:" != *":${INSIGHT_LIBDIR}:"* ]] && LD_LIBRARY_PATH="${INSIGHT_LIBDIR}"

export PATH=$PATH${ORG_PATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}${ORG_LDPATH}


for cfgd in $INSIGHT_USERSHAREDDIR ${INSIGHT_GLOBALSHAREDDIRS/:/ }; do # in that order!
 if [ -d $cfgd/tex ]; then
  export TEXINPUTS=$TEXINPUTS:$cfgd/tex
 fi
done

if [ ! $INSIGHT_GLOBALPYTHONMODULES ]; then
    if which python3 >/dev/null 2>&1; then
     export INSIGHT_PYTHONPATH=$(python3 -c "from distutils import sysconfig; print( sysconfig.get_python_lib( plat_specific=False, prefix='${INSIGHT_INSTDIR}' ) )")
     export PYTHONPATH=$PYTHONPATH:${INSIGHT_PYTHONPATH}
    fi
fi

case "$-" in
 # interactive shell
 *i*) source insight_aliases.sh ;;
esac

