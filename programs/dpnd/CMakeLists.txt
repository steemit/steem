add_executable( dpnd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling dpnd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( DPN_STATIC_BUILD )
   target_link_libraries( dpnd PRIVATE
      "-static-libstdc++ -static-libgcc"
      appbase
      dpn_utilities
      dpn_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
else( DPN_STATIC_BUILD )
   target_link_libraries( dpnd PRIVATE
      appbase
      dpn_utilities
      dpn_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
endif( DPN_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      dpnd PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   dpnd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
