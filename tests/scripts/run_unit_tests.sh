#!/usr/bin/env bash
set -x  # exit on first failure

## default PATH is /usr/local/steemd/bin/
BIN_PATH=${BIN_PATH:-/usr/local/steemd/bin}
echo "Running unit tests (BIN_PATH=${BIN_PATH})..."

tests_to_run=(
    "test_fixed_string"
    "test_block_log"
    "test_sqrt"
    "size_checker"
    "schema_test"
    "js_operation_serializer"
    "get_dev_key"
)

for test in "${tests_to_run[@]}"; do
    echo "Running unit test: $test"
    "${BIN_PATH}/${test}"
done

echo "Running additional unit tests with ctest..."
cd build
ctest -j4 --output-on-failure
./chain_test -t basic_tests/curation_weight_test

echo "All unit tests passed successfully."