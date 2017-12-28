if(MSVC)
    set(hardfork_hpp_file "${CMAKE_CURRENT_SOURCE_DIR}/include/golos/chain/hardfork.hpp")
    add_custom_target(build_hardfork_hpp COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file})
    add_dependencies(build_hardfork_hpp cat-parts)
else(MSVC)
    set(hardfork_hpp_file "${CMAKE_CURRENT_BINARY_DIR}/include/golos/chain/hardfork.hpp")
    add_custom_target(build_hardfork_hpp
            COMMAND "${CMAKE_SOURCE_DIR}/programs/build_helpers/cat_parts.py" "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file})
endif(MSVC)


set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/include/golos/chain/hardfork.hpp" PROPERTIES GENERATED TRUE)

## SORT .cpp by most likely to change / break compile

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_chain SHARED

            # As database takes the longest to compile, start it first
            database.cpp
            fork_database.cpp

            steem_evaluator.cpp

            steem_objects.cpp
            shared_authority.cpp
            #        transaction_object.cpp
            block_log.cpp

            include/golos/chain/account_object.hpp
            include/golos/chain/block_log.hpp
            include/golos/chain/block_summary_object.hpp
            include/golos/chain/comment_object.hpp
            include/golos/chain/compound.hpp
            include/golos/chain/custom_operation_interpreter.hpp
            include/golos/chain/database.hpp
            include/golos/chain/database_exceptions.hpp
            include/golos/chain/db_with.hpp
            include/golos/chain/evaluator.hpp
            include/golos/chain/evaluator_registry.hpp
            include/golos/chain/fork_database.hpp
            include/golos/chain/generic_custom_operation_interpreter.hpp
            include/golos/chain/global_property_object.hpp
            include/golos/chain/history_object.hpp
            include/golos/chain/immutable_chain_parameters.hpp
            include/golos/chain/index.hpp
            include/golos/chain/node_property_object.hpp
            include/golos/chain/operation_notification.hpp
            include/golos/chain/shared_authority.hpp
            include/golos/chain/shared_db_merkle.hpp
            include/golos/chain/snapshot_state.hpp
            include/golos/chain/steem_evaluator.hpp
            include/golos/chain/steem_object_types.hpp
            include/golos/chain/steem_objects.hpp
            include/golos/chain/transaction_object.hpp
            include/golos/chain/witness_objects.hpp

            ${hardfork_hpp_file}
            "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp"
            )
else()
    add_library(golos_chain STATIC

            # As database takes the longest to compile, start it first
            database.cpp
            fork_database.cpp

            steem_evaluator.cpp

            steem_objects.cpp
            shared_authority.cpp
            #        transaction_object.cpp
            block_log.cpp

            include/golos/chain/account_object.hpp
            include/golos/chain/block_log.hpp
            include/golos/chain/block_summary_object.hpp
            include/golos/chain/comment_object.hpp
            include/golos/chain/compound.hpp
            include/golos/chain/custom_operation_interpreter.hpp
            include/golos/chain/database.hpp
            include/golos/chain/database_exceptions.hpp
            include/golos/chain/db_with.hpp
            include/golos/chain/evaluator.hpp
            include/golos/chain/evaluator_registry.hpp
            include/golos/chain/fork_database.hpp
            include/golos/chain/generic_custom_operation_interpreter.hpp
            include/golos/chain/global_property_object.hpp
            include/golos/chain/history_object.hpp
            include/golos/chain/immutable_chain_parameters.hpp
            include/golos/chain/index.hpp
            include/golos/chain/node_property_object.hpp
            include/golos/chain/operation_notification.hpp
            include/golos/chain/shared_authority.hpp
            include/golos/chain/shared_db_merkle.hpp
            include/golos/chain/snapshot_state.hpp
            include/golos/chain/steem_evaluator.hpp
            include/golos/chain/steem_object_types.hpp
            include/golos/chain/steem_objects.hpp
            include/golos/chain/transaction_object.hpp
            include/golos/chain/witness_objects.hpp

            ${hardfork_hpp_file}
            "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp"
            )
endif()

add_dependencies(golos_chain golos_protocol build_hardfork_hpp)
target_link_libraries(golos_chain golos_protocol fc chainbase ${PATCH_MERGE_LIB})
target_include_directories(golos_chain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include")

if(MSVC)
    set_source_files_properties(database.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        golos_chain

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/golos/chain")
