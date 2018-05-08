find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

add_library(bmic_manager_tests INTERFACE )

target_include_directories( bmic_manager_tests INTERFACE ./ )
