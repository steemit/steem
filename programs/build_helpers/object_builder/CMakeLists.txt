file(GLOB HEADERS "include/steemit/object_builder/*.hpp")

add_executable( object_builder

   builder_state.cpp
   full_object_descriptor.cpp
   full_object_descriptor_context.cpp

   object_builder.cpp

   ${HEADERS}
   )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_link_libraries( object_builder PRIVATE fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( object_builder
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
