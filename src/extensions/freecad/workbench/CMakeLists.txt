project(InsightWorkbench)

SET(InsightWorkbench_SOURCES
    Init.py
    InitGui.py
    Insight.py
    InsightFeatures.py
    Insight_rc.py
)

add_custom_target(InsightWorkbench ALL
  COMMENT "Copying files for InsightWorkbench")

# copy script files to appropriate location
# run FreeCAD -M <cmake build dir>/Mod/Insight to have module available
foreach(SRC_FILE ${InsightWorkbench_SOURCES})
  set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE}")
  set(DST "${CMAKE_BINARY_DIR}/Mod/Insight/${SRC_FILE}")

  message(${SRC} ${DST})
  
  add_custom_command(
    TARGET InsightWorkbench
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
  )
endforeach(SRC_FILE)
