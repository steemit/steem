set(CURRENT_TARGET network)

list(APPEND ${CURRENT_TARGET}_HEADERS
        include/golos/network/config.hpp
        include/golos/network/core_messages.hpp
        include/golos/network/exceptions.hpp
        include/golos/network/message.hpp
        include/golos/network/message_oriented_connection.hpp
        include/golos/network/node.hpp
        include/golos/network/peer_connection.hpp
        include/golos/network/peer_database.hpp
        include/golos/network/stcp_socket.hpp
        )

list(APPEND ${CURRENT_TARGET}_SOURCES
        core_messages.cpp
        message_oriented_connection.cpp
        node.cpp
        peer_connection.cpp
        peer_database.cpp
        stcp_socket.cpp
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

target_link_libraries(golos_${CURRENT_TARGET} PUBLIC fc golos_protocol)
target_include_directories(golos_${CURRENT_TARGET}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/include"
        #PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../version/include"
        )

if(MSVC)
    set_source_files_properties(node.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

if(USE_PCH)
    set_target_properties(golos_${CURRENT_TARGET} PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    cotire(golos::network)
endif(USE_PCH)

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${${CURRENT_TARGET}_HEADERS} DESTINATION "include/golos/${CURRENT_TARGET}")

