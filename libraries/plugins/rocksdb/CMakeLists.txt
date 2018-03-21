file(GLOB HEADERS "include/steem/plugins/rocksdb/*.hpp")

add_library( rocksdb_plugin
             rocksdb_plugin.cpp
             rocksdb_api.cpp
           )

target_link_libraries( rocksdb_plugin
   chain_plugin steem_chain steem_protocol json_rpc_plugin rocksdb
   )

target_include_directories( rocksdb_plugin
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
          "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/rocksdb/include"
   )

install( TARGETS
   rocksdb_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
