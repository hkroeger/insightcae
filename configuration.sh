
# the superbuild branch should be identical with the InsightCAE branch
BRANCH=next-release
TRACK_BRANCH=next-release

DOCKERFILE="insightcae-buildsystem_ubuntu-jammy.docker"
DO_WINDOWS_BUILD=0

INSTALL_PREFIX=/opt/insightcae

# array of additional CMAKE options
CMAKE_OPTS=( 
 "-DINSIGHTSB_DEVELOPMENT_BUILD=ON"
 "-DINSIGHTSB_INCLUDE_FX32=ON"
 "-DINSIGHTSB_INCLUDE_FX41=ON"
 "-DINSIGHTSB_INCLUDE_OFESI2112=ON"
 "-DINSIGHTSB_INCLUDE_GMSH=ON"
 "-DINSIGHTSB_USE_SYSTEM_PYTHON=OFF"
 "-DINSIGHTSB_INCLUDE_CODE_ASTER=ON"
 "-DINSIGHTSB_INCLUDE_CGAL=ON"
)

# customer repository info, needed for windows installer
REPO_CUSTOMER="ce-dev"
REPO_PASSWORD=""