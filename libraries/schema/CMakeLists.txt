file(GLOB HEADERS "include/dpn/schema/*.hpp")
add_library( dpn_schema schema.cpp ${HEADERS} )
target_link_libraries( dpn_schema fc )
target_include_directories( dpn_schema PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   dpn_schema

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/dpn/schema" )
