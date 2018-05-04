find_package(libmongoc-1.0 1.8)

if (libmongoc-1.0_FOUND)

  # Golos has no direct dependencies on libmongoc but its shared libraries
  # will need to be present at runtime for the C++ libraries we use:
  # libbsoncxx & libmongocxx (both from github.com/mongodb/mongo-cxx-driver)

  # The *.cmake package files provided by mongo-cxx-driver don't give us the
  # absolute path to the libraries, which is needed whenever they are not
  # installed in system-known locations.  CMake requires the absolute paths
  # in target_link_libraries() since we are builiding an archive and the
  # link step for all executables using this archive must include the
  # mongo-cxx-driver libraries libmongocxx and libbsoncxx.

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
  message("Could NOT find MongoDB. mongo_db_plugin with MongoDB support will not be included.")
  # sudo apt-get install pkg-config libssl-dev libsasl2-dev
  # wget https://github.com/mongodb/mongo-c-driver/releases/download/1.8.0/mongo-c-driver-1.8.0.tar.gz
  # tar xzf mongo-c-driver-1.8.0.tar.gz
  # cd mongo-c-driver-1.8.0
  # ./configure --disable-automatic-init-and-cleanup --enable-static
  # make
  # sudo make install
  #
  # git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
  # cd mongo-cxx-driver/build
  # cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
  # sudo make EP_mnmlstc_core
  # make
  # sudo make install
  #
  # sudo apt-get install mongodb
endif()
