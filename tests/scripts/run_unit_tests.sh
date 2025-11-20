#!/usr/bin/env bash
set -ex  # exit on first failure

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
)

for test in "${tests_to_run[@]}"; do
    echo "Running unit test: $test"
    "${BIN_PATH}/${test}"
done

# Test get_dev_key utility
expected="test_data/get_dev_key_test.jsonl"
actual_output="/tmp/get_dev_key_output.jsonl"

# Run command and save output
"${BIN_PATH}/get_dev_key" xyz wit-block-signing-0:101 > "$actual_output"

# Normalize JSON by sorting keys
jq -S . "$expected" > expected.sorted
jq -S . "$actual_output" > actual.sorted

# Compare
if diff -u expected.sorted actual.sorted > /dev/null; then
    echo "✓ get_dev_key output matches test.jsonl"
else
    echo "✗ get_dev_key output does NOT match test.jsonl"
    diff -u expected.sorted actual.sorted
    exit 1
fi

cd build
ctest -j4 --output-on-failure
./chain_test -t basic_tests/curation_weight_test
