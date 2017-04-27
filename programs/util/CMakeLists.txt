add_executable( get_dev_key get_dev_key.cpp )

target_link_libraries( get_dev_key
                       PRIVATE steemit_app steemit_chain steemit_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   get_dev_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)


add_executable( test_shared_mem test_shared_mem.cpp )

target_link_libraries( test_shared_mem
                       PRIVATE steemit_app steemit_chain steemit_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

add_executable( sign_digest sign_digest.cpp )

target_link_libraries( sign_digest
                       PRIVATE steemit_app steemit_chain steemit_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   sign_digest

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( sign_transaction sign_transaction.cpp )

target_link_libraries( sign_transaction
                       PRIVATE steemit_app steemit_chain steemit_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   sign_transaction

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( inflation_model inflation_model.cpp )
target_link_libraries( inflation_model
                       PRIVATE steemit_chain steemit_protocol fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   inflation_model

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

#add_executable( schema_test schema_test.cpp )
#target_link_libraries( schema_test
#                       PRIVATE steemit_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
#
#install( TARGETS
#   schema_test
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)

add_executable( test_block_log test_block_log.cpp )
target_link_libraries( test_block_log
                       PRIVATE steemit_chain steemit_protocol fc ${CMAKE_DL_LIB} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   test_block_log

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( test_fixed_string test_fixed_string.cpp )
target_link_libraries( test_fixed_string
                       PRIVATE steemit_chain steemit_protocol fc ${CMAKE_DL_LIB} ${PLATFORM_SPECIFIC_LIBS} )

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
