set(CURRENT_TARGET json_rpc_plugin)

list(APPEND CURRENT_TARGET_HEADERS
     include/golos/plugins/json_rpc/plugin.hpp
     include/golos/plugins/json_rpc/utility.hpp
     )

list(APPEND CURRENT_TARGET_SOURCES
     plugin.cpp
     )

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_${CURRENT_TARGET} SHARED
                ${CURRENT_TARGET_HEADERS}
                ${CURRENT_TARGET_SOURCES}
                )
else()
    add_library(golos_${CURRENT_TARGET} STATIC
                ${CURRENT_TARGET_HEADERS}
                ${CURRENT_TARGET_SOURCES}
                )
endif()

add_library(golos::${CURRENT_TARGET} ALIAS golos_${CURRENT_TARGET})
set_property(TARGET golos_${CURRENT_TARGET} PROPERTY EXPORT_NAME ${CURRENT_TARGET})
target_link_libraries(golos_${CURRENT_TARGET} appbase fc)
target_include_directories(golos_${CURRENT_TARGET}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../")

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
