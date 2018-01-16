#Configure needed build options of RocksDB.
SET(WITH_TESTS OFF CACHE BOOL "build with tests")
SET(WITH_SNAPPY ON CACHE BOOL "build with SNAPPY")
SET(WITH_ZLIB ON CACHE BOOL "build with ZLIB")
SET(WITH_BZ2 ON CACHE BOOL "build with BZ2")

add_subdirectory(rocksdb)

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
          "${CMAKE_CURRENT_SOURCE_DIR}/rocksdb/include"
   )

install( TARGETS
   rocksdb_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
