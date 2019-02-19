project(pdl)

set(pdl_SOURCES
    pdl.cpp
    parserdatabase.cpp
    boolparameterparser.cpp
    intparameterparser.cpp
    doubleparameterparser.cpp
    vectorparameterparser.cpp
    doublerangeparameterparser.cpp
    stringparameterparser.cpp
    pathparameterparser.cpp
    selectionparameterparser.cpp
    dynamicclassselectablesubsetparameterparser.cpp
    dynamicclassparametersselectablesubsetparameterparser.cpp
    includedsubsetparameterparser.cpp
    subsetparameterparser.cpp
    matrixparameterparser.cpp
    arrayparameterparser.cpp
    selectablesubsetparameterparser.cpp
)

add_executable(pdl ${pdl_SOURCES})
target_link_libraries(pdl ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

install(TARGETS pdl RUNTIME DESTINATION bin)
