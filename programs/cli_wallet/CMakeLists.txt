add_executable(cli_wallet main.cpp)
if(UNIX AND NOT APPLE)
    set(rt_library rt)
endif()

if(APPLE)
    list(APPEND PLATFORM_SPECIFIC_LIBS readline)
endif()

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

# I don't know why golos_app is required twice in the following line, I just know the linker breaks if it isn't.
target_link_libraries(cli_wallet
        PRIVATE golos_app graphene_net golos_chain golos_protocol graphene_utilities golos_wallet golos_private_message golos_app golos_follow fc readline ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

if(MSVC)
    set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        cli_wallet

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
