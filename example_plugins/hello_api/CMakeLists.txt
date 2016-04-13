file(GLOB HEADERS "include/steemit/plugins/hello_api/*.hpp")

add_library( hello_api
             ${HEADERS}
             hello_api_plugin.cpp
           )

target_link_libraries( hello_api steemit_app steemit_chain fc graphene_db )
target_include_directories( hello_api
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
