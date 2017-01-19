add_executable(get_dev_key get_dev_key.cpp)

target_link_libraries(get_dev_key
        PRIVATE golos_app golos_chain golos_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

install(TARGETS
        get_dev_key

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )


add_executable(test_shared_mem test_shared_mem.cpp)

target_link_libraries(test_shared_mem
        PRIVATE golos_app golos_chain golos_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

add_executable(sign_digest sign_digest.cpp)

target_link_libraries(sign_digest
        PRIVATE golos_app golos_chain golos_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

install(TARGETS
        sign_digest

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

add_executable(sign_transaction sign_transaction.cpp)

target_link_libraries(sign_transaction
        PRIVATE golos_app golos_chain golos_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

install(TARGETS
        sign_transaction

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

add_executable(inflation_model inflation_model.cpp)
target_link_libraries(inflation_model
        PRIVATE golos_chain golos_protocol fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

install(TARGETS
        inflation_model

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

#add_executable( schema_test schema_test.cpp )
#target_link_libraries( schema_test
#                       PRIVATE golos_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
#
#install( TARGETS
#   schema_test
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)

add_executable(test_block_log test_block_log.cpp)
target_link_libraries(test_block_log
        PRIVATE golos_chain golos_protocol fc ${CMAKE_DL_LIB} ${PLATFORM_SPECIFIC_LIBS})

install(TARGETS
        test_block_log

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
