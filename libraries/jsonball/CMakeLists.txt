
#
# The general workflow here is that some JSON is included in the json/ directory
# and that JSON is made available to the C++ at runtime
# This code is copied from libraries/manifest
#
# Some scripts in here also produce JSON
#

##############################################################################
# First, we run scripts that produce JSON and put the output in the
# template_context directory for later inclusion in templates.
#

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
   python3 -m steem_build_helpers.mm_generate_ticks
   -o ${CMAKE_CURRENT_BINARY_DIR}/template_context/mm_ticks.json
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not generate market maker ticks")
endif()

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
   python3 -m steem_build_helpers.rc_generate_resource_parameters
   -i ${CMAKE_CURRENT_SOURCE_DIR}/data/resource_parameters.json
   -o ${CMAKE_CURRENT_BINARY_DIR}/template_context/resource_parameters.json
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not generate market maker ticks")
endif()

##############################################################################
# We instantiate jinja2 templates with the JSON values created earlier.
# We use execute_process() to do dry runs at configure time to figure
# out the dependencies and outputs.
# Then we use add_custom_command() to execute at build time.

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
   python3 -m steem_build_helpers.buildj2
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
   --print-dependencies
   OUTPUT_VARIABLE TEMPLATE_DEPS
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not get template dependencies")
endif()
message( STATUS "TEMPLATE_DEPS: ${TEMPLATE_DEPS}" )

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
   python3 -m steem_build_helpers.buildj2
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
   --print-outputs
   OUTPUT_VARIABLE TEMPLATE_OUTPUTS
   RESULT_VARIABLE RETURN_VALUE
)

if( NOT RETURN_VALUE EQUAL 0 )
   message(FATAL_ERROR "Could not get template outputs")
endif()
message( STATUS "TEMPLATE_OUTPUTS: ${TEMPLATE_OUTPUTS}" )

add_custom_command(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
   python3 -m steem_build_helpers.buildj2
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
   OUTPUT ${TEMPLATE_OUTPUTS}
   DEPENDS ${TEMPLATE_DEPS}
)

# Repeat the buildj2 as execute_process so CMakeLists.txt exists at configure time

execute_process(
   COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
   python3 -m steem_build_helpers.buildj2
   -j ${CMAKE_CURRENT_BINARY_DIR}/template_context
   -t ${CMAKE_CURRENT_SOURCE_DIR}/templates
   -o ${CMAKE_CURRENT_BINARY_DIR}/gensrc
)

add_subdirectory( ${CMAKE_CURRENT_BINARY_DIR}/gensrc/jsonball ${CMAKE_CURRENT_BINARY_DIR}/genbuild )
