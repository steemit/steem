file(GLOB HEADERS "include/steemit/plugins/auth_util/*.hpp")

add_library( steemit_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( steemit_auth_util steemit_app steemit_chain steemit_protocol fc )
target_include_directories( steemit_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
