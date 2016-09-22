file(GLOB HEADERS "include/graphene/db2/*.hpp")
add_library( graphene_db2 database.cpp ${HEADERS} )
target_link_libraries( graphene_db2 fc )
target_include_directories( graphene_db2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_db2

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/db" )
