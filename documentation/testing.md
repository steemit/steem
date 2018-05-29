# Testing

The unit test target is `make chain_test`
This creates an executable `./tests/chain_test` that will run all unit tests.

Tests are broken in several categories:
```
basic_tests          // Tests of "basic" functionality
block_tests          // Tests of the block chain
live_tests           // Tests on live chain data (currently only past hardfork testing)
operation_tests      // Unit Tests of Golos operations
operation_time_tests // Tests of Golos operations that include a time based component (ex. vesting withdrawals)
serialization_tests  // Tests related of serialization
```

## Configuration (draft)

Some config options:
* `log_level` — allows setting the log level. Available values: `all`, `success`, `test_suite`, `message`, `warning`, . `error`, `cpp_exception`, `system_error`, `fatal_error`, `nothing`. Sample usage: `--log-level=test_suite`
* `report_level` — allows setting the level of detailization for the testing results report. Available values: `no`, `confirm`, `short`, `detailed`. Sample usage: `--report_level=detailed`
* `run_test` — specifies which test units to run. You can specify individual test (separated with comma) or test cases. Sample usage: `--run_test=operation_tests/delegation`

For more info about runtime config check [Boost.Tests documentation](https://www.boost.org/doc/libs/1_58_0/libs/test/doc/html/utf/user-guide/runtime-config/reference.html).


# Code Coverage Testing

If you have not done so, install lcov `brew install lcov`

```
cmake -D BUILD_GOLOS_TESTNET=ON -D ENABLE_COVERAGE_TESTING=true -D CMAKE_BUILD_TYPE=Debug .
make
lcov --capture --initial --directory . --output-file base.info --no-external
tests/chain_test
lcov --capture --directory . --output-file test.info --no-external
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info
lcov -o interesting.info -r total.info tests/\*
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix `pwd`
```

Now open `lcov/index.html` in a browser
