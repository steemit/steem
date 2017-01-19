file(GLOB HEADERS "include/steemit/app/*.hpp")

add_library(golos_app
        database_api.cpp
        api.cpp
        application.cpp
        impacted.cpp
        plugin.cpp
        ${HEADERS}
        )

target_link_libraries(golos_app golos_chain golos_protocol golos_tags golos_follow golos_mf_plugins fc graphene_net graphene_time graphene_utilities)
target_include_directories(golos_app
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
    set_source_files_properties(application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        golos_app

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/steemit/app")
