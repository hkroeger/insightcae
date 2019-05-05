
# autocompletion
function _functionObjects() {
 local sel cur
 sel=$(ls -1 postProcessing)
 cur="${COMP_WORDS[COMP_CWORD]}"
 COMPREPLY=( $(compgen -W "$sel" -- ${cur}) )
 return 0
}


alias isplrb='for t in $(cd postProcessing/rigidBodyMotion; ls -1|sort -g); do cat postProcessing/rigidBodyMotion/$t/rigidBodyMotion.dat; done|isofPlotTabular -'

function isplf { for t in $(cd postProcessing/$1; ls -1|sort -g); do cat postProcessing/$1/$t/force.dat; done | isofPlotTabular -; }
complete -F _functionObjects isplf
