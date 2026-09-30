project(remotetest_common)

# shared infrastructure for the remote execution tests
# (test/unit/remote, test/integration/remote, test/integration/workbench)

if (wt_FOUND AND TARGET dummies)

    add_library(remotetest_common STATIC
        remotetest.h remotetest.cpp
        localshellserver.h localshellserver.cpp
        backends.h backends.cpp
        fakeanalyzeserver.h fakeanalyzeserver.cpp
    )
    set_target_properties(remotetest_common PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(remotetest_common PUBLIC toolkit toolkit_remote dummies)
    target_include_directories(remotetest_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


    # remote_test(NAME
    #   [SOURCES src...]    default: NAME.cpp
    #   [LIBS lib...]
    #   [ARGS arg...]
    #   [LABELS label...]
    #   [FIXTURES fixture...]
    #   [TIMEOUT seconds]   default: 300
    #   [TEST_NAME name]    default: remote_NAME
    #   [NO_EXECUTABLE]     reuse an already defined executable NAME )
    #
    # Creates executable testexe_NAME and registers it as a test.
    # All tests get SKIP_RETURN_CODE 77 and the environment variable
    # INSIGHT_TEST_ANALYZE_EXE pointing to the analyze executable.
    function(remote_test NAME)
        cmake_parse_arguments(RT
            "NO_EXECUTABLE"
            "TIMEOUT;TEST_NAME"
            "SOURCES;LIBS;ARGS;LABELS;FIXTURES"
            ${ARGN})

        if (NOT RT_SOURCES)
            set(RT_SOURCES ${NAME}.cpp)
        endif()
        if (NOT RT_TIMEOUT)
            set(RT_TIMEOUT 300)
        endif()
        if (NOT RT_TEST_NAME)
            set(RT_TEST_NAME remote_${NAME})
        endif()

        if (NOT RT_NO_EXECUTABLE)
            add_executable(testexe_${NAME} ${RT_SOURCES})
            target_link_libraries(testexe_${NAME} remotetest_common ${RT_LIBS})
            linkToolkitVtk(testexe_${NAME} Offscreen)
            add_dependencies(testexe_${NAME} analyze)
        endif()

        add_test(NAME ${RT_TEST_NAME} COMMAND testexe_${NAME} ${RT_ARGS})
        set_tests_properties(${RT_TEST_NAME} PROPERTIES
            SKIP_RETURN_CODE 77
            TIMEOUT ${RT_TIMEOUT}
            LABELS "remote;${RT_LABELS}"
            ENVIRONMENT "INSIGHT_TEST_ANALYZE_EXE=$<TARGET_FILE:analyze>;QT_QPA_PLATFORM=offscreen"
        )
        if (RT_FIXTURES)
            set_tests_properties(${RT_TEST_NAME} PROPERTIES
                FIXTURES_REQUIRED "${RT_FIXTURES}")
        endif()
    endfunction()


    # remote_backend_test(NAME [LIBS ...] [LABELS ...] [TIMEOUT s] [SSH_ONLY])
    #
    # registers one test per backend:
    #  NAME_localshell      always (unless SSH_ONLY)
    #  NAME_ssh_localhost   requires the "localhost" remote server (fixture remote_ssh)
    #  NAME_ssh_env         opt-in: INSIGHT_TEST_SSH_HOST, skipped otherwise
    #  NAME_wsl_env         opt-in: INSIGHT_TEST_WSL_DISTRO (Windows), skipped otherwise
    function(remote_backend_test NAME)
        cmake_parse_arguments(RBT "SSH_ONLY" "TIMEOUT" "LIBS;LABELS" ${ARGN})
        if (NOT RBT_TIMEOUT)
            set(RBT_TIMEOUT 300)
        endif()

        add_executable(testexe_${NAME} ${NAME}.cpp)
        target_link_libraries(testexe_${NAME} remotetest_common ${RBT_LIBS})
        linkToolkitVtk(testexe_${NAME} Offscreen)
        add_dependencies(testexe_${NAME} analyze)

        if (NOT RBT_SSH_ONLY)
            remote_test(${NAME} NO_EXECUTABLE
                TEST_NAME ${NAME}_localshell
                ARGS --backend localshell
                LABELS ${RBT_LABELS} localshell
                TIMEOUT ${RBT_TIMEOUT})
        endif()

        remote_test(${NAME} NO_EXECUTABLE
            TEST_NAME ${NAME}_ssh_localhost
            ARGS --backend ssh-localhost
            LABELS ${RBT_LABELS} ssh
            FIXTURES remote_ssh
            TIMEOUT ${RBT_TIMEOUT})

        remote_test(${NAME} NO_EXECUTABLE
            TEST_NAME ${NAME}_ssh_env
            ARGS --backend ssh-env
            LABELS ${RBT_LABELS} ssh optin
            TIMEOUT ${RBT_TIMEOUT})

        if (NOT RBT_SSH_ONLY)
            remote_test(${NAME} NO_EXECUTABLE
                TEST_NAME ${NAME}_wsl_env
                ARGS --backend wsl-env
                LABELS ${RBT_LABELS} wsl optin
                TIMEOUT ${RBT_TIMEOUT})
        endif()
    endfunction()


    # precondition for all tests on the "localhost" SSH server
    remote_test(check_localhost_remote
        TEST_NAME remote_check_localhost_remote
        LABELS ssh
        TIMEOUT 120)
    set_tests_properties(remote_check_localhost_remote PROPERTIES
        FIXTURES_SETUP remote_ssh)

    set(INSIGHT_REMOTE_TESTS_ENABLED ON PARENT_SCOPE)

else()
    message(STATUS "Wt or insight-dummies addon not available: remote execution tests disabled")
endif()
