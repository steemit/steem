file(GLOB HEADERS "include/graphene/schema/*.hpp")
add_library( graphene_schema schema.cpp ${HEADERS} )
target_link_libraries( graphene_schema fc )
target_include_directories( graphene_schema PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_schema

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/schema" )
