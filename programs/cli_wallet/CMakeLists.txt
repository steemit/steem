add_executable( cli_wallet main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( cli_wallet
                       PRIVATE steemit_app graphene_net steemit_chain graphene_utilities steemit_wallet steemit_private_message steemit_follow fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   cli_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
