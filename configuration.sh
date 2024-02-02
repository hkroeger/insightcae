
# the superbuild branch should be identical with the InsightCAE branch
BRANCH=master
TRACK_BRANCH=master
OS=ubuntu
VER=jammy
DO_WINDOWS_BUILD=1

INSTALL_PREFIX=/opt/insightcae

# array of additional CMAKE options
CMAKE_OPTS=( 
 "-DINSIGHT_BUILD_MEDREADER=ON"
 "-DINSIGHTSB_INCLUDE_FX32=ON"
 "-DINSIGHTSB_INCLUDE_FX41=ON"
 "-DINSIGHTSB_INCLUDE_OFESI2112=ON"
 "-DINSIGHTSB_INCLUDE_GMSH=ON"
 "-DINSIGHTSB_USE_SYSTEM_PYTHON=OFF"
 "-DINSIGHTSB_INCLUDE_CODE_ASTER=ON"
 "-DINSIGHTSB_INCLUDE_CGAL=ON"
)

# customer repository info, needed for windows installer
REPO_CUSTOMER="ce"
REPO_PASSWORD=""
