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
    if ! "${BIN_PATH}/${test}"; then
        echo "Unit test $test failed!"
        exit 1
    fi
done

# run get_dev_key separately
echo "Running unit test: get_dev_key"
if ! "${BIN_PATH}/get_dev_key" xyz "wit-block-signing-0:101"; then
    echo "Unit test get_dev_key failed!"
    exit 1
fi

echo "Running additional unit tests with ctest..."
cd "${BUILD_PATH}/tests"
ls -lh ./

if ! ctest -j4 --output-on-failure; then
    echo "ctest unit tests failed!"
    exit 1
fi

if ! ./chain_test -t basic_tests/curation_weight_test; then
    echo "chain_test -t basic_tests/curation_weight_test failed!"
    exit 1
fi

echo "All unit tests passed successfully."
exit 0
