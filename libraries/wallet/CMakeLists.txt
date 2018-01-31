set(CURRENT_TARGET wallet)

find_package(Perl)
find_package(Doxygen)

if(PERL_FOUND AND DOXYGEN_FOUND AND NOT "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${DOXYGEN_EXECUTABLE}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/include/steemit/wallet/wallet.hpp)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
            COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new

            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
            COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm)
else()
    # no perl and doxygen, generate the best docs we can at runtime from reflection
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp)
endif()

list(APPEND ${CURRENT_TARGET}_HEADERS
        include/golos/wallet/api_documentation.hpp
        include/golos/wallet/reflect_util.hpp
        include/golos/wallet/remote_node_api.hpp
        include/golos/wallet/wallet.hpp
)

list(APPEND ${CURRENT_TARGET}_SOURCES
        api_documentation_standin.cpp
        remote_node_api.cpp
        wallet.cpp
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
# I don't know why golos_app is required twice in the following line, I just know the linker breaks if it isn't.
target_link_libraries(
        golos_${CURRENT_TARGET}
        PRIVATE
        golos_network
        golos_chain
        golos::network_broadcast_api
        golos::database_api
        golos::account_history
        golos::market_history
        golos::social_network
        golos::private_message
        golos::follow
        golos::account_by_key
        golos_protocol
        graphene_utilities
        fc
        ${CMAKE_DL_LIBS}
        ${PLATFORM_SPECIFIC_LIBS}
)
target_include_directories( golos_${CURRENT_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
    set_source_files_properties(wallet.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/golos/wallet")
