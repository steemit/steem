#!/usr/bin/env bash
set -e  # exit on first failure

if [ "$UNIT_TEST" = "ON" ]; then
    echo "Running unit tests..."

    cd tests
    ctest -j4 --output-on-failure
    ./chain_test -t basic_tests/curation_weight_test
    cd ..

    ./programs/util/test_fixed_string
    ./programs/util/test_block_log
    ./programs/util/test_sqrt
    ./programs/size_checker
    ./programs/util/schema_test
    ./programs/js_operation_serializer


    expected="test_data/get_dev_key_test.jsonl"
    actual_output="/tmp/get_dev_key_output.jsonl"

    # Run command and save output
    ./programs/util/get_dev_key xyz wit-block-signing-0:101 > "$actual_output"

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
else
    echo "UNIT_TEST is not ON — skipping tests."
fi
