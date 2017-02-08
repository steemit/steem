file(GLOB HEADERS "include/steemit/delayed_node/*.hpp")

add_library(golos_delayed_node
        delayed_node_plugin.cpp
        )

target_link_libraries(golos_delayed_node golos_chain golos_protocol golos_app)
target_include_directories(golos_delayed_node
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
    set_source_files_properties(delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        golos_delayed_node

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
