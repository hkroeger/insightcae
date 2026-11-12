if (INSIGHT_BUILD_ISCAD)

project(iscad)

find_package(Qt5Network CONFIG REQUIRED)

# chat assistant: language model access and agent loop (Qt only, unit tested separately)
add_library(iscadassistant STATIC
    assistant/llmtypes.h
    assistant/llmbackend.h
    assistant/ssestreamparser.h assistant/ssestreamparser.cpp
    assistant/streamingbackend.h assistant/streamingbackend.cpp
    assistant/anthropicbackend.h assistant/anthropicbackend.cpp
    assistant/openaicompatbackend.h assistant/openaicompatbackend.cpp
    assistant/iscadassistanthost.h
    assistant/iscadassistantagent.h assistant/iscadassistantagent.cpp
)
target_enable_qt(iscadassistant)
set_target_properties(iscadassistant PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(iscadassistant PUBLIC Qt5::Core Qt5::Network)
target_include_directories(iscadassistant PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# the ISCAD language documentation is compiled in as reference for the assistant
set(ISCAD_ASSISTANT_DOC_DIR ${CMAKE_SOURCE_DIR}/doc/docs/cad)
set(ISCAD_ASSISTANT_QRC ${CMAKE_CURRENT_BINARY_DIR}/iscad_assistant_doc.qrc)
if (EXISTS ${ISCAD_ASSISTANT_DOC_DIR}/insightcad.md)
    file(WRITE ${ISCAD_ASSISTANT_QRC}.tmp
"<RCC><qresource prefix=\"/iscad_assistant\">
<file alias=\"insightcad.md\">${ISCAD_ASSISTANT_DOC_DIR}/insightcad.md</file>
<file alias=\"iscad-reference.md\">${ISCAD_ASSISTANT_DOC_DIR}/iscad-reference.md</file>
</qresource></RCC>
")
else()
    message(WARNING "ISCAD documentation not found in ${ISCAD_ASSISTANT_DOC_DIR}: the chat assistant will work without language reference")
    file(WRITE ${ISCAD_ASSISTANT_QRC}.tmp "<RCC><qresource prefix=\"/iscad_assistant\"></qresource></RCC>\n")
endif()
# only touch the resource file on changes (avoids recompilation on each configure)
configure_file(${ISCAD_ASSISTANT_QRC}.tmp ${ISCAD_ASSISTANT_QRC} COPYONLY)


add_executable(iscad
    iscad.cpp

    iqiscadmainwindow.cpp iqiscadmainwindow.h
    iqiscadmodelwindow.h iqiscadmodelwindow.cpp
    iqiscadmodelscriptedit.cpp iqiscadmodelscriptedit.h
    iqiscadsyntaxhighlighter.cpp iqiscadsyntaxhighlighter.h
    iqiscadfoldingmargin.cpp iqiscadfoldingmargin.h
    iqiscadcompleter.cpp iqiscadcompleter.h
    scriptpositionmap.cpp scriptpositionmap.h

    modelcomponentselectordlg.cpp modelcomponentselectordlg.h
    insertfeaturedlg.cpp insertfeaturedlg.h
    loadmodeldialog.cpp loadmodeldialog.h
    drawingviewsmodel.h drawingviewsmodel.cpp
    viewdefinitiondialog.h viewdefinitiondialog.cpp
    insertdrawingdialog.h insertdrawingdialog.cpp
    iqiscadassistanthost.h iqiscadassistanthost.cpp
    iqiscadchatwidget.h iqiscadchatwidget.cpp
    iqiscadassistantsettings.h iqiscadassistantsettings.cpp

    modelcomponentselectordlg.ui
    insertfeaturedlg.ui
    loadmodeldialog.ui
    insertdrawingdialog.ui
    viewdefinitiondialog.ui

    iscad.qrc
    ${ISCAD_ASSISTANT_QRC}
)
target_enable_qt(iscad)
target_link_libraries(iscad
  Qt5::Widgets
  iscadassistant
  toolkit_gui
  ${DXFLIB_LIBRARIES}
  )
linkToolkitVtk(iscad Onscreen)

target_setup_gettext(iscad iscad)

target_include_directories(iscad
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS iscad RUNTIME DESTINATION bin)
endif()
