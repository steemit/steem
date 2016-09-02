file(GLOB HEADERS "include/graphene/net/*.hpp")

set(SOURCES node.cpp
            stcp_socket.cpp
            core_messages.cpp
            peer_database.cpp
            peer_connection.cpp
            message_oriented_connection.cpp)

add_library( graphene_net ${SOURCES} ${HEADERS} )

target_link_libraries( graphene_net 
  PUBLIC fc graphene_db )
target_include_directories( graphene_net 
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_SOURCE_DIR}/libraries/chain/include"
)

if(MSVC)
  set_source_files_properties( node.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

if (USE_PCH)
  set_target_properties(graphene_net PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(graphene_net)
endif(USE_PCH)

install( TARGETS
   graphene_net

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/net" )
