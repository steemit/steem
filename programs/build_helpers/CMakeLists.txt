
add_executable( cat-parts cat-parts.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

# we only actually need Boost, but link against FC for now so we don't duplicate it.
target_link_libraries( cat-parts PRIVATE fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
