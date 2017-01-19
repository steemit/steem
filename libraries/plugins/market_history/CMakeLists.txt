file(GLOB HEADERS "include/steemit/market_history/*.hpp")

add_library(golos_market_history
        market_history_plugin.cpp
        market_history_api.cpp
        )

target_link_libraries(golos_market_history golos_chain golos_protocol golos_app)
target_include_directories(golos_market_history
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_market_history

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )