# Defines Golos library target.
project(Golos)
cmake_minimum_required(VERSION 2.8.12)

set(BLOCKCHAIN_NAME "Golos")

set(GUI_CLIENT_EXECUTABLE_NAME Golos)
set(CUSTOM_URL_SCHEME "gcs")
set(INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347")

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must DB at least 4.8!")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version must be at least 3.3!")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

#set (ENABLE_INSTALLER 1)
#set (USE_PCH 1)

if(USE_PCH)
    include(cotire)
endif(USE_PCH)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fc/CMakeModules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fc/GitVersionGen")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA2)

set(BOOST_COMPONENTS)
list(APPEND BOOST_COMPONENTS thread
        date_time
        system
        filesystem
        program_options
        signals
        serialization
        chrono
        unit_test_framework
        context
        locale
)

option(Boost_USE_STATIC_LIBS "Build with Boost static libraries usage" TRUE)

option(BUILD_SHARED_LIBRARIES "Build shared libraries" FALSE)

option(BUILD_GOLOS_TESTNET "Build source for test network (ON OR OFF)" OFF)
message(STATUS "BUILD_GOLOS_TESTNET: ${BUILD_GOLOS_TESTNET}")
if(BUILD_GOLOS_TESTNET)
    message(STATUS "     ")
    message(STATUS "             CONFIGURING FOR TEST NET             ")
    message(STATUS "     ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTEEMIT_BUILD_TESTNET")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTEEMIT_BUILD_TESTNET")
endif()

option(LOW_MEMORY_NODE "Build source for low memory node (ON OR OFF)" FALSE)
message(STATUS "LOW_MEMORY_NODE: ${LOW_MEMORY_NODE}")
if(LOW_MEMORY_NODE)
    message(STATUS "     ")
    message(STATUS "             CONFIGURING FOR LOW MEMORY NODE      ")
    message(STATUS "     ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIS_LOW_MEM")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIS_LOW_MEM")
endif()

option(CHAINBASE_CHECK_LOCKING "Check locks in chainbase (ON or OFF)" TRUE)
message(STATUS "CHAINBASE_CHECK_LOCKING: ${CHAINBASE_CHECK_LOCKING}")
if(CHAINBASE_CHECK_LOCKING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCHAINBASE_CHECK_LOCKING")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCHAINBASE_CHECK_LOCKING")
endif()

option(CLEAR_VOTES "Build source to clear old votes from memory" ON)
if(CLEAR_VOTES)
    message(STATUS "   CONFIGURING TO CLEAR OLD VOTES FROM MEMORY")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLEAR_VOTES")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCLEAR_VOTES")
endif()

if(WIN32)
    set(BOOST_ROOT $ENV{BOOST_ROOT})
    set(Boost_USE_MULTITHREADED ON)
    set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
endif(WIN32)

find_package(Boost 1.57 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
# For Boost 1.53 on windows, coroutine was not in BOOST_LIBRARYDIR and do not need it to build,  but if boost versin >= 1.54, find coroutine otherwise will cause link errors
if(NOT "${Boost_VERSION}" MATCHES "1.53(.*)")
    set(BOOST_LIBRARIES_TEMP ${Boost_LIBRARIES})
    find_package(Boost 1.54 REQUIRED COMPONENTS coroutine)
    list(APPEND BOOST_COMPONENTS coroutine)
    set(Boost_LIBRARIES ${BOOST_LIBRARIES_TEMP} ${Boost_LIBRARIES})
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(WIN32)

    message(STATUS "Configuring Golos on WIN32")
    set(DB_VERSION 60)
    set(BDB_STATIC_LIBS 1)

    set(ZLIB_LIBRARIES "")
    set(DEFAULT_EXECUTABLE_INSTALL_DIR bin/)

    set(CRYPTO_LIB)

    if(MSVC)
        add_compile_options(/wd4503 /wd4267 /wd4244)
        #looks like this flag can have different default on some machines.
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

        # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

        # On windows tcl should be installed to the directory pointed by setenv.bat script
        set(TCL_INCLUDE_PATH $ENV{TCL_ROOT}/include)
        message(STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}")

        find_package(TCL)
        message(STATUS "tcl_library: ${TCL_LIBRARY}")

        set(TCL_LIBS "optimized;${TCL_LIBRARY};debug;")
        get_filename_component(TCL_LIB_PATH "${TCL_LIBRARY}" PATH)
        get_filename_component(TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE)
        get_filename_component(TCL_LIB_EXT "${TCL_LIBRARY}" EXT)

        set(TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}")
        set(TCL_LIBRARY ${TCL_LIBS})

    elseif(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive -msse4.2 -Wa,-mbig-obj")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
        # Optimization flag apparently needed to get rid of "File too big" assembler errors when compiling in Debug mode
        # See: https://stackoverflow.com/questions/14125007/gcc-string-table-overflow-error-during-compilation/14601779#29479701
        set(CMAKE_CXX_FLAGS_DEBUG "-O2")
        if(FULL_STATIC_BUILD)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif(FULL_STATIC_BUILD)
    endif(MSVC)

else(WIN32) # Apple AND Linux

    find_library(READLINE_LIBRARIES NAMES readline)
    find_path(READLINE_INCLUDE_DIR readline/readline.h)
    #if(NOT READLINE_INCLUDE_DIR OR NOT READLINE_LIBRARIES)
    #  MESSAGE(FATAL_ERROR "Could not find lib readline.")
    #endif()

    if(APPLE)
        # Apple Specific Options Here
        message(STATUS "Configuring Golos on OS X")
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -stdlib=libc++ -Wall -Wno-conversion -Wno-deprecated-declarations")
    else(APPLE)
        # Linux Specific Options Here
        message(STATUS "Configuring Golos on Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall")
        set(rt_library rt)
        set(pthread_library pthread)
        if(NOT DEFINED crypto_library)
            # I'm not sure why this is here, I guess someone has openssl and can't detect it with find_package()?
            # if you have a normal install, you can define crypto_library to the empty string to avoid a build error
            set(crypto_library crypto)
        endif()
        if(FULL_STATIC_BUILD)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif(FULL_STATIC_BUILD)
    endif(APPLE)

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp")
    endif()

    if("${CMAKE_GENERATOR}" STREQUAL "Ninja")
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
        endif()
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

    # based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
    # set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif(WIN32)

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build Golos for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
    set(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

# external_plugins needs to be compiled first because libraries/app depends on GOLOSIT_EXTERNAL_PLUGINS being fully populated
add_subdirectory(thirdparty)
add_subdirectory(libraries)
add_subdirectory(plugins)
add_subdirectory(programs)
#add_subdirectory(tests)

if(ENABLE_INSTALLER)

    set(VERSION_MAJOR 0)
    set(VERSION_MINOR 1)
    set(VERSION_PATCH 0)


    include(InstallRequiredSystemLibraries)

    set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
    set(CPACK_PACKAGE_NAME "golos")
    set(CPACK_PACKAGE_VENDOR "Golos, Inc.")
    set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
    set(CPACK_PACKAGE_DESCRIPTION "A client for the Golos network")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A client for the Golos network")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Golos ${CPACK_PACKAGE_VERSION}")

    if(WIN32)
        set(CPACK_GENERATOR "ZIP;NSIS")
        set(CPACK_PACKAGE_NAME "Golos") # override above
        set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
        set(CPACK_NSIS_PACKAGE_NAME "Golos v${CPACK_PACKAGE_VERSION}")
        set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}")
        set(CPACK_NSIS_DEFINES "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"Golos\\\"")
        # it seems like windows zip files usually don't have a single directory inside them, unix tgz frequently do
        set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

    endif(WIN32)

    if(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
    endif()

    if(LINUX)
        # Linux gets a .tgz
        set(CPACK_GENERATOR "TGZ")
        set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
    endif(LINUX)

    include(CPack)
endif(ENABLE_INSTALLER)

if(BUILD_GOLOS_TESTNET)
    message(STATUS "\n\n             CONFIGURED FOR TEST NETWORK             \n\n")
else()
    message(STATUS "\n\n             CONFIGURED FOR GOLOS NETWORK             \n\n")
endif()

if(LOW_MEMORY_NODE)
    message(STATUS "\n\n             CONFIGURED FOR LOW MEMORY NODE          \n\n")
else()
    message(STATUS "\n\n             CONFIGURED FOR FULL NODE                \n\n")
endif()


