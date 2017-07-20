file(GLOB HEADERS "include/steemit/plugins/market_history/*.hpp")

add_library( market_history_plugin
             market_history_plugin.cpp
           )

target_link_libraries( market_history_plugin chain_plugin )
target_include_directories( market_history_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   market_history_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)