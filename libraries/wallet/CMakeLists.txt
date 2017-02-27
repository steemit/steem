file(GLOB HEADERS "include/steemit/wallet/*.hpp")

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

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_wallet SHARED wallet.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp ${HEADERS})
else()
    add_library(golos_wallet STATIC wallet.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp ${HEADERS})
endif()
# I don't know why golos_app is required twice in the following line, I just know the linker breaks if it isn't.
target_link_libraries(golos_wallet PRIVATE golos_app graphene_net golos_chain golos_protocol graphene_utilities fc golos_private_message golos_app golos_follow golos_account_by_key ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})
target_include_directories(golos_wallet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
    set_source_files_properties(wallet.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        golos_wallet

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/steemit/wallet")
