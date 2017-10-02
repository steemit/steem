#!/bin/bash
time ./tests/chain_test --run_test=operation_time_tests/performance_account_std_stress
time ./tests/chain_test --run_test=operation_time_tests/performance_account_interprocess_stress

