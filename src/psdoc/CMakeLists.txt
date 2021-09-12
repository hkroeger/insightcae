project(psdoc)

set(psdoc_SOURCES
    main.cpp
    latextable.h latextable.cpp
    latexdocumentation.h latexdocumentation.cpp
)

add_executable(psdoc ${psdoc_SOURCES})
target_link_libraries(psdoc ${Boost_LIBRARIES})
linkToolkitVtk(psdoc Offscreen)

install(TARGETS psdoc RUNTIME DESTINATION bin)
