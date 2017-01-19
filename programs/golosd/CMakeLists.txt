add_executable(golosd main.cpp)
if(UNIX AND NOT APPLE)
    set(rt_library rt)
endif()

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling golosd with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

target_link_libraries(golosd
        PRIVATE golos_external_plugins golos_internal_plugins golos_mf_plugins golos_app golos_witness golos_account_history golos_chain golos_protocol fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

install(TARGETS
        golosd

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
