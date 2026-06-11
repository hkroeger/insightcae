if (NOT WIN32)

project(pdl)

set(pdl_SOURCES
    pdl.cpp
    parametergenerator.cpp parametergenerator.h
    boolgenerator.cpp boolgenerator.h
    intparameterparser.cpp intparameterparser.h
    doubleparameterparser.cpp doubleparameterparser.h
    vectorparameterparser.cpp vectorparameterparser.h
    doublerangegenerator.cpp doublerangegenerator.h
    stringparameterparser.cpp stringparameterparser.h
    pathgenerator.cpp pathgenerator.h
    directorygenerator.h directorygenerator.cpp
    selectiongenerator.cpp selectiongenerator.h
    dynamicclassselectablesubsetparameterparser.cpp dynamicclassselectablesubsetparameterparser.h
    dynamicclassparametersselectablesubsetgenerator.cpp dynamicclassparametersselectablesubsetgenerator.h
    includedsubsetparameterparser.cpp includedsubsetparameterparser.h
    subsetgenerator.cpp subsetgenerator.h
    matrixparameterparser.cpp matrixparameterparser.h
    arraygenerator.cpp arraygenerator.h
    selectablesubsetgenerator.cpp selectablesubsetgenerator.h
    propertylibraryselectiongenerator.h propertylibraryselectiongenerator.cpp
    spatialtransformationparameterparser.h spatialtransformationparameterparser.cpp

    labeledarraygenerator.h labeledarraygenerator.cpp
    labeledarraykeyselectiongenerator.h labeledarraykeyselectiongenerator.cpp
    cadgeometrygenerator.h cadgeometrygenerator.cpp
    cadsketchgenerator.h cadsketchgenerator.cpp
    dateparameterparser.h dateparameterparser.cpp
    datetimeparameterparser.h datetimeparameterparser.cpp
    parametersetgenerator.h parametersetgenerator.cpp
)

add_executable(pdl ${pdl_SOURCES})
target_link_libraries(pdl
    Boost::regex
    Boost::system
    Boost::filesystem
    Boost::date_time
    Boost::program_options
)
target_compile_definitions(pdl
    PRIVATE
        BOOST_SPIRIT_USE_PHOENIX_V3
        BOOST_NO_SCOPED_ENUMS
        BOOST_NO_CXX11_SCOPED_ENUMS
    )
install(TARGETS pdl RUNTIME DESTINATION bin)

endif()
