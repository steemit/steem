file(GLOB HEADERS "include/steemit/manifest/*.hpp")

# helpful hints from
# https://blog.kangz.net/posts/2016/05/26/integrating-a-code-generator-with-cmake/
# https://stackoverflow.com/questions/26074450/how-to-include-generated-files-for-compilation-with-cmake

##############################################################################
# First, we generate plugins.json by reading all the plugins.json files.
# We run the Python script once at configure time with execute_process()
# to figure out which files will be read (using the --print-dependencies option),
# then we run it again at build time with add_custom_command() to
# actually read the files.
#
# This way it will be properly regenerated.
#

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.plugins
   ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
   -o ${CMAKE_CURRENT_BINARY_DIR}/template_context/plugins.json
   --print-dependencies
   OUTPUT_VARIABLE PLUGINS_DEPS
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not get plugin dependencies")
endif()
message( STATUS "PLUGINS_DEPS: ${PLUGINS_DEPS}" )

add_custom_command(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.plugins
   ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
   -o ${CMAKE_CURRENT_BINARY_DIR}/template_context/plugins.json
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/template_context/plugins.json
   DEPENDS ${PLUGINS_DEPS} ${CMAKE_CURRENT_SOURCE_DIR}/steem_manifest/plugins.py
)

##############################################################################
# We instantiate jinja2 templates with the values in plugins.json.
# Just like generating plugins.json, we use execute_process() to do dry runs
# at configure time to figure out the dependencies and outputs.
# Then we use add_custom_command() to execute at build time.

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.build
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
   --print-dependencies
   OUTPUT_VARIABLE MANIFEST_DEPS
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not get manifest dependencies")
endif()
message( STATUS "MANIFEST_DEPS: ${MANIFEST_DEPS}" )

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.build
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
   --print-outputs
   OUTPUT_VARIABLE MANIFEST_OUTPUTS
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not get manifest outputs")
endif()
message( STATUS "MANIFEST_OUTPUTS: ${MANIFEST_OUTPUTS}" )

add_custom_command(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.build
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
   OUTPUT ${MANIFEST_OUTPUTS}
   DEPENDS ${MANIFEST_DEPS} ${CMAKE_CURRENT_SOURCE_DIR}/steem_manifest/build.py
)

##############################################################################
# Now we need to repeat the two above add_custom_command() as execute_process()
# because CMakeLists.txt is one of the generated files, and it must be
# created at configure time!
#

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.plugins
   ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
   -o ${CMAKE_CURRENT_BINARY_DIR}/template_context/plugins.json
)

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
   python3 -m steem_manifest.build
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
)

add_subdirectory( ${CMAKE_CURRENT_BINARY_DIR}/gensrc/plugins ${CMAKE_CURRENT_BINARY_DIR}/genbuild )
