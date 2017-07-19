add_executable( steemd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling steemd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( steemd PRIVATE
   steemit_plugins
   steemit_mf_plugins
   steemit_app
   steemit_witness
   steemit_account_history
   steemit_chain
   steemit_protocol
   fc
   ${CMAKE_DL_LIBS}
   ${PLATFORM_SPECIFIC_LIBS}
)

install( TARGETS
   steemd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
