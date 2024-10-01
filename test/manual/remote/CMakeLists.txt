if (wt_FOUND)

  macro(remote_test NAME)
    add_executable(${NAME} ${NAME}.cpp)
    target_link_libraries(${NAME} toolkit toolkit_remote)
    linkToolkitVtk(${NAME} Offscreen)

    add_test(manual_remote_${NAME} ${NAME})
  endmacro()

  remote_test(remoteserver_allocation)
  remote_test(remoteserverpool_request)
  remote_test(poolallocation_reconnection)
  remote_test(remoteserver_commandexecution)
  target_link_libraries(remoteserver_commandexecution dummies)
endif()
