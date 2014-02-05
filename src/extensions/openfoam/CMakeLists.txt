 
find_package(OpenFOAM-16ext REQUIRED)
find_package(OpenFOAM-22x REQUIRED)

add_subdirectory(FEMDisplacementBC)
add_subdirectory(scripts)
add_subdirectory(simpleDyMFoam)
add_subdirectory(convertPressure)
add_subdirectory(kOmegaSST2)