set(CURRENT_TARGET database_api_plugin)

list(APPEND ${CURRENT_TARGET}_HEADERS
     include/golos/plugins/database_api/applied_operation.hpp
     include/golos/plugins/database_api/state.hpp
     include/golos/plugins/database_api/plugin.hpp


     include/golos/plugins/database_api/api_objects/account_api_object.hpp
     include/golos/plugins/database_api/api_objects/account_recovery_request_api_object.hpp
     include/golos/plugins/database_api/api_objects/feed_history_api_object.hpp
     include/golos/plugins/database_api/forward.hpp
     include/golos/plugins/database_api/api_objects/owner_authority_history_api_object.hpp
     include/golos/plugins/database_api/api_objects/savings_withdraw_api_object.hpp
     include/golos/plugins/database_api/api_objects/witness_api_object.hpp


     )

list(APPEND ${CURRENT_TARGET}_SOURCES
     api.cpp
     applied_operation.cpp
)

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_${CURRENT_TARGET} SHARED
                ${${CURRENT_TARGET}_HEADERS}
                ${${CURRENT_TARGET}_SOURCES}
                )
else()
    add_library(golos_${CURRENT_TARGET} STATIC
                ${${CURRENT_TARGET}_HEADERS}
                ${${CURRENT_TARGET}_SOURCES}
                )
endif()

add_library(golos::${CURRENT_TARGET} ALIAS golos_${CURRENT_TARGET})
set_property(TARGET golos_${CURRENT_TARGET} PROPERTY EXPORT_NAME ${CURRENT_TARGET})

target_link_libraries(
        golos_${CURRENT_TARGET}
        golos_chain
        golos::chain_plugin
        golos_protocol
        golos::json_rpc_plugin
        graphene_utilities
        appbase
        fc
)
target_include_directories(golos_${CURRENT_TARGET} PUBLIC "include")

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )