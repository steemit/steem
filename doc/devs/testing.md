# Testing

The unit test target is `make chain_test`
This creates an executable `./tests/chain_test` that will run all unit tests.

Tests are broken in several categories:
```
basic_tests          // Tests of "basic" functionality
block_tests          // Tests of the block chain
live_tests           // Tests on live chain data (currently only past hardfork testing)
operation_tests      // Unit Tests of Steem operations
operation_time_tests // Tests of Steem operations that include a time based component (ex. vesting withdrawals)
serialization_tests  // Tests related of serialization
```

# Code Coverage Testing

If you have not done so, install lcov `brew install lcov`

```
cmake -D BUILD_STEEM_TESTNET=ON -D ENABLE_COVERAGE_TESTING=true -D CMAKE_BUILD_TYPE=Debug .
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
