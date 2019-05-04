
# autocompletion
function _functionObjects() {
 local sel cur
 sel=$(ls -1 postProcessing)
 cur="${COMP_WORDS[COMP_CWORD]}"
 COMPREPLY=( $(compgen -W "$sel" -- ${cur}) )
 return 0
}


alias isplrb='cat postProcessing/rigidBodyMotion/*/rigidBodyMotion.dat | isofPlotTabular -'

function isplf { ( cat postProcessing/$1/*/force.dat | isofPlotTabular - ) }
complete -F _functionObjects isplf
