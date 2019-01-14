if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    SET(INSIGHT_DESKTOP_DIR      "/usr/share/applications")
    SET(INSIGHT_PIXMAPS_DIR      "/usr/share/icons")
    SET(INSIGHT_SERVICEMENUS_DIR "/usr/share/kde4/services/ServiceMenus")

    #SET(INSIGHT_USER_DESKTOP_DIR     "~/.local/share/applications")
    SET(INSIGHT_USER_PIXMAPS_DIR      "~/.local/share/icons")
    SET(INSIGHT_USER_SERVICEMENUS_DIR "~/.kde/share/kde4/services/ServiceMenus")

    # try to set up the menu system
    find_program(XDG-MIME_EXECUTABLE xdg-mime)
    find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
    find_program(XDG-ICON-RESOURCE_EXECUTABLE xdg-icon-resource)
    find_program(DESKTOP-FILE-INSTALL_EXECUTABLE desktop-file-install)

    if (INSIGHT_BUILD_INSTALLDESKTOP)
        add_custom_target(desktop_integration ALL)
    endif()

    macro(XDG_INSTALL_MIME ICON_FILE MIME_FILE MIME_TYPE)
        # Copy pixmap
        INSTALL (FILES ${ICON_FILE} DESTINATION ${INSIGHT_PIXMAPS_DIR})
        INSTALL (FILES mime/${MIME_FILE} DESTINATION ${INSIGHT_DESKTOP_DIR})

        INSTALL(CODE "
          execute_process(COMMAND ${XDG-ICON-RESOURCE_EXECUTABLE} install --size 256 --context mimetypes ${INSIGHT_PIXMAPS_DIR}/${ICON_FILE})
          execute_process(COMMAND ${XDG-MIME_EXECUTABLE} install --novendor ${INSIGHT_DESKTOP_DIR}/${MIME_FILE})
          "
        )

        if (INSIGHT_BUILD_INSTALLDESKTOP)
            add_custom_command(
                TARGET desktop_integration PRE_BUILD
                 COMMENT "Installing desktop integration in user space"
                 COMMAND ${XDG-ICON-RESOURCE_EXECUTABLE} install --size 256 --context mimetypes ${CMAKE_CURRENT_LIST_DIR}/${ICON_FILE}
                 COMMAND ${XDG-MIME_EXECUTABLE} install --novendor ${CMAKE_CURRENT_LIST_DIR}/mime/${MIME_FILE}
                )
        endif()
    endmacro()

    macro(XDG_INSTALL ICON_FILE MIME_FILE MIME_TYPE DESKTOP_FILE)
        # Copy pixmap
        INSTALL (FILES ${ICON_FILE} DESTINATION ${INSIGHT_PIXMAPS_DIR})
        INSTALL (FILES mime/${MIME_FILE} DESTINATION ${INSIGHT_DESKTOP_DIR})
        INSTALL (FILES ${DESKTOP_FILE} DESTINATION ${INSIGHT_DESKTOP_DIR})

        INSTALL(CODE "
          execute_process(COMMAND ${XDG-ICON-RESOURCE_EXECUTABLE} install --size 256 --context mimetypes ${INSIGHT_PIXMAPS_DIR}/${ICON_FILE})
          execute_process(COMMAND ${XDG-MIME_EXECUTABLE} install --novendor ${INSIGHT_DESKTOP_DIR}/${MIME_FILE})
          execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${INSIGHT_DESKTOP_DIR}/${DESKTOP_FILE})
          execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default ${INSIGHT_DESKTOP_DIR}/${DESKTOP_FILE} ${MIME_TYPE})
          "
        )

        if (INSIGHT_BUILD_INSTALLDESKTOP)
            add_custom_command(
                TARGET desktop_integration PRE_BUILD
                 COMMENT "Installing desktop integration in user space"
                 COMMAND ${XDG-ICON-RESOURCE_EXECUTABLE} install --size 256 --context mimetypes ${CMAKE_CURRENT_LIST_DIR}/${ICON_FILE}
                 COMMAND ${XDG-MIME_EXECUTABLE} install --novendor ${CMAKE_CURRENT_LIST_DIR}/mime/${MIME_FILE}
                 COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${CMAKE_CURRENT_LIST_DIR}/${DESKTOP_FILE}
                 COMMAND ${XDG-MIME_EXECUTABLE} default ${CMAKE_CURRENT_LIST_DIR}/${DESKTOP_FILE} ${MIME_TYPE}
                )
        endif()
    endmacro()

    macro(INSTALL_KDE_SERVICEMENU DESKTOP_FILE)
        # Copy pixmap
        INSTALL (FILES ${DESKTOP_FILE} DESTINATION ${INSIGHT_SERVICEMENUS_DIR})

        if (INSIGHT_BUILD_INSTALLDESKTOP)
            add_custom_command(
                TARGET desktop_integration PRE_BUILD
                 COMMENT "Installing desktop integration in user space"
                 #COMMAND ${CMAKE_COMMAND} -E make_directory ${INSIGHT_USER_SERVICEMENUS_DIR}
                 #COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/${DESKTOP_FILE} ${INSIGHT_USER_SERVICEMENUS_DIR}
                 COMMAND ${DESKTOP-FILE-INSTALL_EXECUTABLE} --dir=${INSIGHT_USER_SERVICEMENUS_DIR} ${CMAKE_CURRENT_LIST_DIR}/${DESKTOP_FILE}
                )
        endif()
    endmacro()

    XDG_INSTALL(application-insight-ist.png insight-ist.xml application/insight-ist insight-ist.desktop)
    XDG_INSTALL(application-insight-ist.png insight-iscb.xml application/insight-iscb insight-iscb.desktop)
    XDG_INSTALL_MIME(application-insight-ist.png insight-foam.xml application/insight-foam)
    #INSTALL_KDE_SERVICEMENU(insight-ist-service.desktop)
    #INSTALL_KDE_SERVICEMENU(insight-foam.desktop)

    XDG_INSTALL(application-insight-iscad.png insight-iscad.xml application/insight-iscad insight-iscad.desktop)
endif()
