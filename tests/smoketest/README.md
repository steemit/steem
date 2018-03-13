# Quick Basic Test Suit (AKA smoketest) Documentation

## To Run Smoketest

From directory of smoketest.sh:

    ./smoketest.sh ~/working/steemd ~/reference/steemd ~/working_blockchain_dir ~/reference_blockchain_dir 3000000

## To Add a Test Group

In directory of smoketest.sh:
1. Create subdirectory with group name.
2. In that subdirectory create test_group.sh which
   i Accepts following arguments: JOBS TEST_ADDRESS TEST_PORT REF_ADDRESS REF_PORT BLOCK_LIMIT where
      JOBS is suggested number of parallel jobs to run,
      TEST_ADDRESS:TEST_PORT indicate where the tested steemd instance listens to API requests,
      REF_ADDRESS:REF_PORT ditto for reference instance and
      BLOCK_LIMIT is the top number of block to be processed.
   ii Returns 0 on success, non-zero on failure