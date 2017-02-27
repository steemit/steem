file(GLOB HEADERS "include/steemit/protocol/*.hpp")

## SORT .cpp by most likely to change / break compile
if(BUILD_SHARED_LIBRARIES)
    add_library(golos_protocol SHARED

            types.cpp
            authority.cpp
            operations.cpp
            sign_state.cpp
            operation_util_impl.cpp
            steem_operations.cpp
            transaction.cpp
            block.cpp
            asset.cpp
            version.cpp
            get_config.cpp

            ${HEADERS}
            )
else()
    add_library(golos_protocol STATIC

            types.cpp
            authority.cpp
            operations.cpp
            sign_state.cpp
            operation_util_impl.cpp
            steem_operations.cpp
            transaction.cpp
            block.cpp
            asset.cpp
            version.cpp
            get_config.cpp

            ${HEADERS}
            )
endif()

target_link_libraries(golos_protocol fc)
target_include_directories(golos_protocol
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include")

install(TARGETS
        golos_protocol

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

install(FILES ${HEADERS} DESTINATION "include/steemit/protocol")
