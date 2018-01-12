set(CURRENT_TARGET protocol)

list(APPEND ${CURRENT_TARGET}_HEADERS
        include/golos/protocol/asset.hpp
        include/golos/protocol/authority.hpp
        include/golos/protocol/base.hpp
        include/golos/protocol/block.hpp
        include/golos/protocol/block_header.hpp
        include/golos/protocol/config.hpp
        include/golos/protocol/exceptions.hpp
        include/golos/protocol/get_config.hpp
        include/golos/protocol/operation_util.hpp
        include/golos/protocol/operation_util_impl.hpp
        include/golos/protocol/operations.hpp
        include/golos/protocol/protocol.hpp
        include/golos/protocol/sign_state.hpp
        include/golos/protocol/steem_operations.hpp
        include/golos/protocol/steem_virtual_operations.hpp
        include/golos/protocol/transaction.hpp
        include/golos/protocol/types.hpp
        include/golos/protocol/version.hpp
        )

list(APPEND ${CURRENT_TARGET}_SOURCES
        asset.cpp
        authority.cpp
        block.cpp
        get_config.cpp
        operation_util_impl.cpp
        operations.cpp
        sign_state.cpp
        steem_operations.cpp
        transaction.cpp
        types.cpp
        version.cpp
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

#add_dependencies(golos_${CURRENT_TARGET} golos::version)

target_link_libraries(golos_${CURRENT_TARGET} fc) #golos::version)
target_include_directories(golos_${CURRENT_TARGET}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../version/include" "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/../../version/include")

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

install(FILES ${${CURRENT_TARGET}_HEADERS} DESTINATION "include/golos/${CURRENT_TARGET}")
