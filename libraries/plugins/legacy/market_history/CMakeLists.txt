file(GLOB HEADERS "include/steemit/market_history/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_market_history SHARED
            market_history_plugin.cpp
            market_history_api.cpp
            )
else()
    add_library(golos_market_history STATIC
            market_history_plugin.cpp
            market_history_api.cpp
            )
endif()

target_link_libraries(golos_market_history golos_chain golos_protocol golos_app)
target_include_directories(golos_market_history
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_market_history

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )