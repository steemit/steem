file(GLOB HEADERS "include/steemit/plugin/core_info/*.hpp")

add_library( steemit_core_info
             core_info_api.cpp
             core_info_plugin.cpp
           )

target_link_libraries( steemit_core_info steemit_chain steemit_app graphene_time )
target_include_directories( steemit_core_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_core_info

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
