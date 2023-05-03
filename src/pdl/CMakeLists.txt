if (NOT WIN32)

project(pdl)

set(pdl_SOURCES
    pdl.cpp
    parserdatabase.cpp parserdatabase.h
    boolparameterparser.cpp boolparameterparser.h
    intparameterparser.cpp intparameterparser.h
    doubleparameterparser.cpp doubleparameterparser.h
    vectorparameterparser.cpp vectorparameterparser.h
    doublerangeparameterparser.cpp doublerangeparameterparser.h
    stringparameterparser.cpp stringparameterparser.h
    pathparameterparser.cpp pathparameterparser.h
    selectionparameterparser.cpp selectionparameterparser.h
    dynamicclassselectablesubsetparameterparser.cpp dynamicclassselectablesubsetparameterparser.h
    dynamicclassparametersselectablesubsetparameterparser.cpp dynamicclassparametersselectablesubsetparameterparser.h
    includedsubsetparameterparser.cpp includedsubsetparameterparser.h
    subsetparameterparser.cpp subsetparameterparser.h
    matrixparameterparser.cpp matrixparameterparser.h
    arrayparameterparser.cpp arrayparameterparser.h
    selectablesubsetparameterparser.cpp selectablesubsetparameterparser.h
    propertylibraryselectionparameterparser.h propertylibraryselectionparameterparser.cpp
    spatialtransformationparameterparser.h spatialtransformationparameterparser.cpp
    cadgeometryparameterparser.h cadgeometryparameterparser.cpp
)

add_executable(pdl ${pdl_SOURCES})
target_link_libraries(pdl ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})

install(TARGETS pdl RUNTIME DESTINATION bin)

endif()
