file(GLOB HEADERS "include/steem/plugins/hello_api/*.hpp")

add_library( hello_api
             ${HEADERS}
             hello_api_plugin.cpp
           )

target_link_libraries( hello_api steem_app steem_chain fc graphene_db )
target_include_directories( hello_api
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
