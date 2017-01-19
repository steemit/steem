file(GLOB HEADERS "include/steemit/plugins/debug_node/*.hpp")

add_library(golos_debug_node
        ${HEADERS}
        debug_node_plugin.cpp
        debug_node_api.cpp
        )

target_link_libraries(golos_debug_node golos_app golos_chain golos_protocol fc)
target_include_directories(golos_debug_node
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS golos_debug_node
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/steemit/plugins/debug_node")
