project(psdoc)

set(psdoc_SOURCES
    isgenerateparametersetreference.cpp
    latextable.h latextable.cpp
    latexdocumentation.h latexdocumentation.cpp
)

add_executable(isGenerateParameterSetReference ${psdoc_SOURCES})
target_link_libraries(isGenerateParameterSetReference ${Boost_LIBRARIES})
linkToolkitVtk(isGenerateParameterSetReference Offscreen)

install(TARGETS isGenerateParameterSetReference RUNTIME DESTINATION bin)
