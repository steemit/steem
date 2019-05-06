add_executable( get_dev_key get_dev_key.cpp )

target_link_libraries( get_dev_key
                       PRIVATE steem_chain steem_protocol steem_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   get_dev_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)


add_executable( test_shared_mem test_shared_mem.cpp )

target_link_libraries( test_shared_mem
                       PRIVATE  steem_chain steem_protocol steem_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

add_executable( sign_digest sign_digest.cpp )

target_link_libraries( sign_digest
                       PRIVATE steem_chain steem_protocol steem_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   sign_digest

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( sign_transaction sign_transaction.cpp )

target_link_libraries( sign_transaction
                       PRIVATE steem_chain steem_protocol steem_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   sign_transaction

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( serialize_set_properties serialize_set_properties.cpp )

target_link_libraries( serialize_set_properties
                       PRIVATE steem_chain steem_protocol steem_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   serialize_set_properties

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( inflation_model inflation_model.cpp )
target_link_libraries( inflation_model
                       PRIVATE steem_chain steem_protocol fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   inflation_model

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( dump_steem_schema dump_steem_schema.cpp )
target_link_libraries( dump_steem_schema
                       PRIVATE steem_chain steem_protocol fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
install( TARGETS
   dump_steem_schema

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( schema_test schema_test.cpp )
target_link_libraries( schema_test
                       PRIVATE steem_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   schema_test

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( test_block_log test_block_log.cpp )
target_link_libraries( test_block_log
                       PRIVATE steem_chain steem_protocol fc ${CMAKE_DL_LIB} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   test_block_log

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( test_fixed_string test_fixed_string.cpp )
target_link_libraries( test_fixed_string
                       PRIVATE steem_chain steem_protocol fc ${CMAKE_DL_LIB} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   test_fixed_string

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( test_sqrt test_sqrt.cpp )
target_link_libraries( test_sqrt PRIVATE fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
install( TARGETS
   test_sqrt

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
