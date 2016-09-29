
file(GLOB HEADERS "include/graphene/db2/*.hpp")
add_library( steemit_chain2 chain_database.cpp ${HEADERS} )
target_link_libraries( steemit_chain2 steemit_chain fc graphene_db2 steemit_protocol )
target_include_directories( steemit_chain2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_chain2

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/db" )


add_executable( test_chain2 main.cpp )
target_link_libraries( test_chain2 steemit_chain2 steemit_protocol graphene_db2 fc  ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
