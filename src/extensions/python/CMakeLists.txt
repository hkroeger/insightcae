project(insight_python_package)

file(GLOB COPY_FILES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  __init__.py)
  
add_custom_target(insight_python ALL)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py")
set(DST "${CMAKE_BINARY_DIR}/share/insight/python/Insight/__init__.py")

add_custom_command(
  TARGET insight_python
  COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
  )