add_executable( js_operation_serializer main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_link_libraries( js_operation_serializer
                       PRIVATE steemit_app graphene_net steemit_chain steemit_protocol graphene_utilities steemit_wallet fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   js_operation_serializer

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
