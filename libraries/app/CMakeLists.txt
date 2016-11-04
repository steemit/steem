file(GLOB HEADERS "include/steemit/app/*.hpp")

add_library( steemit_app
             database_api.cpp
             api.cpp
             application.cpp
             impacted.cpp
             plugin.cpp
             ${HEADERS}
           )

#
# zlib / bzip2 libraries are actually linked in by FC, because the linker order is very sensitive
# to the order things are passed in, and having them linked as part of FC is the only way
# I found that would successfully link.
#
find_package( ZLIB )
if( ZLIB_FOUND )
  add_definitions( -DHAS_ZLIB )
endif( ZLIB_FOUND )

find_package( BZip2 )
if( BZIP2_FOUND )
   add_definitions( -DHAS_BZIP2 )
endif( BZIP2_FOUND )

target_link_libraries( steemit_app steemit_chain steemit_tags steemit_follow steemit_mf_plugins fc graphene_db graphene_net graphene_time graphene_utilities )
target_include_directories( steemit_app
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   steemit_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/app" )
