file(GLOB HEADERS "include/steemit/witness/*.hpp")

add_library(golos_witness
        witness.cpp
        )

target_link_libraries(golos_witness golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_witness
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_witness

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
