file(GLOB HEADERS "include/dpn/plugins/auth_util/*.hpp")

add_library( dpn_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( dpn_auth_util dpn_app dpn_chain dpn_protocol fc )
target_include_directories( dpn_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
