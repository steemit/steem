#!/usr/bin/env bash
set -e  # exit on first failure

## default PATH is /usr/local/steemd/bin/
BIN_PATH=${BIN_PATH:-/usr/local/steemd/bin}
BUILD_PATH=${BUILD_PATH:-/usr/local/src/build}

echo "Running unit tests (BIN_PATH=${BIN_PATH})..."
ls -lh "${BIN_PATH}"

tests_to_run=(
    "test_fixed_string"
    "test_block_log"
    "test_sqrt"
    "size_checker"
    "schema_test"
    "js_operation_serializer"
)

for test in "${tests_to_run[@]}"; do
    echo "Running unit test: $test"
    "${BIN_PATH}/${test}"
done

# run get_dev_key separately
echo "Running unit test: get_dev_key"
"${BIN_PATH}/get_dev_key" xyz "wit-block-signing-0:101"

echo "Running additional unit tests with ctest..."
cd "${BUILD_PATH}"
ls -lh ./

ctest -j4 --output-on-failure
./chain_test -t basic_tests/curation_weight_test

echo "All unit tests passed successfully."
