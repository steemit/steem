# Quick Basic Test Suit (AKA smoketest) Documentation

## To Run Smoketest

From directory of smoketest.sh (example of cmd line, for more see smoketest.sh):

./smoketest.sh ~/working/steemd ~/reference/steemd ~/working_blockchain_dir ~/reference_blockchain_dir 3000000 12

py scripts generate json response files only if request failed or response is different - look for them in `log`
folder created inside test group.

## To Add a Test Group

In directory of smoketest.sh:
1. Create subdirectory with group name.
1. In that subdirectory create test_group.sh which
    - Accepts following arguments: JOBS TEST_ADDRESS TEST_PORT REF_ADDRESS REF_PORT BLOCK_LIMIT where:
      JOBS is suggested number of parallel jobs to run,
      TEST_ADDRESS:TEST_PORT indicate where the tested steemd instance listens to API requests,
      REF_ADDRESS:REF_PORT ditto for reference instance and
      BLOCK_LIMIT is the top number of block to be processed.
    - Returns 0 on success, non-zero on failure.
    - Create config.ini in test directory or config_ref.ini and config_test.ini for dedicated steemd instances.
      If no config*.ini files you need manually create them in working directories of steemd instances.
