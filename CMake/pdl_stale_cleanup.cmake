# pdl_stale_cleanup.cmake
#
# Removes PDL-generated artifacts for header files that have been removed or
# renamed from a target's SOURCES since the last build.
#
# Called by the ${TARGETNAME}_PDLGenerator custom target after all per-header
# gen-sets.py invocations have completed.
#
# Required variables (passed via -D on the cmake command line):
#   TARGET_BINARY_DIR   - CMAKE_CURRENT_BINARY_DIR of the target
#   TARGET_NAME         - name of the CMake target
#   INCLUDE_MIRROR_DIR  - shared include mirror (${CMAKE_BINARY_DIR}/include/insightcae)
cmake_policy(SET CMP0057 NEW)

set(headers_file "${TARGET_BINARY_DIR}/${TARGET_NAME}_pdl_headers.txt")
set(prev_file    "${TARGET_BINARY_DIR}/${TARGET_NAME}_pdl_headers_prev.txt")

file(STRINGS "${headers_file}" current_basenames)

if(EXISTS "${prev_file}")
    file(STRINGS "${prev_file}" prev_basenames)
else()
    set(prev_basenames "")
endif()

foreach(bn ${prev_basenames})
    if(NOT "${bn}" IN_LIST current_basenames)
        message(STATUS "PDL cleanup: header '${bn}.h' removed from target '${TARGET_NAME}', cleaning artifacts")
        set(mf "${TARGET_BINARY_DIR}/${bn}_pdl.manifest")
        if(EXISTS "${mf}")
            file(STRINGS "${mf}" stems)
            foreach(stem ${stems})
                # Remove the intermediate .pdl file
                if(EXISTS "${TARGET_BINARY_DIR}/${stem}.pdl")
                    file(REMOVE "${TARGET_BINARY_DIR}/${stem}.pdl")
                endif()
                # Remove generated __*.h files locally and from the include mirror
                file(GLOB h_files "${TARGET_BINARY_DIR}/${stem}*.h")
                foreach(hf ${h_files})
                    file(REMOVE "${hf}")
                    get_filename_component(hfn "${hf}" NAME)
                    if(EXISTS "${INCLUDE_MIRROR_DIR}/${hfn}")
                        file(REMOVE "${INCLUDE_MIRROR_DIR}/${hfn}")
                    endif()
                endforeach()
            endforeach()
            file(REMOVE "${mf}")
        endif()
        # Remove the orphaned timestamp file
        if(EXISTS "${TARGET_BINARY_DIR}/${bn}_pdl.timestamp")
            file(REMOVE "${TARGET_BINARY_DIR}/${bn}_pdl.timestamp")
        endif()
    endif()
endforeach()

# Persist the current header list for the next build
file(READ "${headers_file}" _pdl_headers_content)
file(WRITE "${prev_file}" "${_pdl_headers_content}")
