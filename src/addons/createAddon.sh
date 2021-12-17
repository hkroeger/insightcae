#!/bin/bash
set -e

NAME=${NAME:-MyAddon}

usage() { echo "Usage: $0 [-n <lower case add-on name>] [<Add-on name>...]" 1>&2; exit 1; }

while getopts "n:" o; do
    case "${o}" in
         n) name=${OPTARG};;
        *) usage ;;
    esac
done
shift $((OPTIND-1))
NAME=$1


if [ -z "$name" ]; then
 name=$(echo $NAME|tr '[:upper:]' '[:lower:]')
fi

DIR="insight-$name"

echo "Creating add-on $NAME ($name) in directory $DIR..."

(
mkdir $DIR
cd $DIR

touch $name.cpp  $name.h ${name}_gui.cpp ${name}_gui.h


cat > CMakeLists.txt << EOF
project($NAME)

set(INSIGHT_INSTALL_COMPONENT $name)

if (INSIGHT_BUILD_TOOLKIT AND INSIGHT_BUILD_OPENFOAM AND INSIGHT_BUILD_CAD)

    SET(${name}_SOURCES
        $name.cpp
        $name.h
    )

    add_library($name SHARED \${${name}_SOURCES})

    target_link_libraries($name toolkit insightcad)
    target_include_directories($name
        PUBLIC \${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC \${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_PDL(${name} "\${${name}_SOURCES}")

    set(${name}gui_SOURCES
        ${name}_gui.cpp ${name}_gui.h
    )
    add_library(${name}gui SHARED \${${name}gui_SOURCES})
    target_link_libraries(${name}gui ${name} toolkit_gui)

    install(TARGETS ${name} LIBRARY DESTINATION lib COMPONENT \${INSIGHT_INSTALL_COMPONENT})
    install(TARGETS ${name}gui LIBRARY DESTINATION lib COMPONENT \${INSIGHT_INSTALL_COMPONENT})

    install_shared_file2(${name}.module modules.d)
endif()
EOF



cat > $name.module << EOF
library ${name}
guilibrary ${name}gui
EOF


git init .
git add *

)
