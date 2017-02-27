file(GLOB HEADERS "include/graphene/schema/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(graphene_schema SHARED schema.cpp ${HEADERS})
else()
    add_library(graphene_schema STATIC schema.cpp ${HEADERS})
endif()

target_link_libraries(graphene_schema fc)
target_include_directories(graphene_schema PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        graphene_schema

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/graphene/schema")
