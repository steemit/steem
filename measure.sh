#!/bin/bash
time ./tests/chain_test --run_test=operation_time_tests/performance_account_std_stress && mv timelines.txt std_timelines.txt
time ./tests/chain_test --run_test=operation_time_tests/performance_account_interprocess_stress && mv timelines.txt bip_timelines.txt

