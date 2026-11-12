if (INSIGHT_BUILD_CAD)
 install_shared_directory(iscad-library iscad-library iscad-library)
endif()
if (INSIGHT_BUILD_ISCAD)
 install_shared_directory(iscad-assistant-eval-tasks iscad-assistant-eval iscad-assistant-eval)
endif()

add_subdirectory(reference_data)
