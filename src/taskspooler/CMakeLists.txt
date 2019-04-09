project(taskspooler)

set(tsp_SRC
    main.c
    server.c
    server_start.c
    client.c
    msgdump.c
    jobs.c
    execute.c
    msg.c
    mail.c
    error.c
    signals.c
    list.c
    print.c
    info.c
    env.c
    tail.c
)

add_executable(tsp ${tsp_SRC})
install(TARGETS tsp RUNTIME DESTINATION bin)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(tspmon_SRC
    taskspoolermonitor.cpp
)

SET(tspmon_FORMS
    taskspoolermonitor.ui
)

add_library(taskspoolermonitor SHARED ${tspmon_SRC} ${tspmon_FORMS})
target_link_libraries(taskspoolermonitor
 toolkit
 Qt5::Widgets #Qt5::WebEngineWidgets
 ${Boost_LIBRARIES}
 )

target_include_directories(taskspoolermonitor
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS taskspoolermonitor LIBRARY DESTINATION lib)
