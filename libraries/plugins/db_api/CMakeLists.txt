file(GLOB HEADERS "include/steemit/db_api/*.hpp")

add_library( steemit_db_api
             db_api.cpp
             db_api_plugin.cpp
           )

target_link_libraries( steemit_db_api steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_db_api
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_db_api

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
