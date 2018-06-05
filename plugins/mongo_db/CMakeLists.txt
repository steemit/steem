
if(ENABLE_MONGO_PLUGIN)

  # Golos has no direct dependencies on libmongoc but its shared libraries
  # will need to be present at runtime for the C++ libraries we use:
  # libbsoncxx & libmongocxx (both from github.com/mongodb/mongo-cxx-driver)

  # The *.cmake package files provided by mongo-cxx-driver don't give us the
  # absolute path to the libraries, which is needed whenever they are not
  # installed in system-known locations.  CMake requires the absolute paths
  # in target_link_libraries() since we are builiding an archive and the
  # link step for all executables using this archive must include the
  # mongo-cxx-driver libraries libmongocxx and libbsoncxx.

  find_package(libmongoc-1.0 1.8)

  if (libmongoc-1.0_FOUND)

    message("MongoDB found, building plugin...")

    find_package(libbsoncxx REQUIRED)
    find_package(libmongocxx REQUIRED)
  
    set(CURRENT_TARGET mongo_db)
  
    list(APPEND CURRENT_TARGET_HEADERS
      include/golos/plugins/mongo_db/mongo_db_plugin.hpp
      include/golos/plugins/mongo_db/mongo_db_writer.hpp
      include/golos/plugins/mongo_db/mongo_db_operations.hpp
      include/golos/plugins/mongo_db/mongo_db_state.hpp
      include/golos/plugins/mongo_db/mongo_db_types.hpp
      )

    list(APPEND CURRENT_TARGET_SOURCES
      mongo_db_plugin.cpp
      mongo_db_writer.cpp
      mongo_db_operations.cpp
      mongo_db_state.cpp
      mongo_db_types.cpp
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
    
    target_link_libraries(
      golos_${CURRENT_TARGET}
      golos::json_rpc
      golos_chain
      golos::chain_plugin
      golos::follow
      appbase
      fc
      ${LIBBSONCXX_LIBRARIES}
      ${LIBMONGOCXX_LIBRARIES}
    )
    target_include_directories(golos_${CURRENT_TARGET} PUBLIC "include" ${LIBMONGOCXX_INCLUDE_DIRS} ${LIBBSONCXX_INCLUDE_DIRS})
  
    install(TARGETS
      golos_${CURRENT_TARGET}
  
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
    )
    install( FILES ${HEADERS} DESTINATION "include/golos/plugins/mongo_db" )
  else()
    message("Mongo drivers NOT found. Use proper Dockerfile")
  endif()
endif()