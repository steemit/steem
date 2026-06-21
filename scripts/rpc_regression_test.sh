#!/bin/bash
# RPC Regression Test for PR #3712
# Tests both Test A and Test B nodes with 10 RPC calls each

set -euo pipefail

NODE_A="http://localhost:18091"
NODE_B="http://localhost:28091"

PASS=0
FAIL=0

rpc_call() {
  local endpoint="$1"
  local method="$2"
  local params="${3:-}"
  local payload

  if [ -n "$params" ]; then
    payload="{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"$method\",\"params\":$params}"
  else
    payload="{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"$method\"}"
  fi

  curl -s --max-time 15 -d "$payload" "$endpoint"
}

check_result() {
  local test_name="$1"
  local node="$2"
  local result="$3"
  local check_expr="$4"

  if echo "$result" | eval "$check_expr" > /dev/null 2>&1; then
    echo "  PASS: [$node] $test_name"
    ((PASS++))
  else
    echo "  FAIL: [$node] $test_name"
    echo "    Response: $(echo "$result" | head -c 200)"
    ((FAIL++))
  fi
}

echo "========================================"
echo "RPC Regression Test - PR #3712"
echo "Test A: $NODE_A"
echo "Test B: $NODE_B"
echo "Started: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo "========================================"
echo ""

for NODE_URL in "$NODE_A" "$NODE_B"; do
  NODE_NAME=$(echo "$NODE_URL" | grep -oP '\d+$')

  echo "--- Testing Node on port $NODE_NAME ---"

  # Test 1: get_dynamic_global_properties
  RESULT=$(rpc_call "$NODE_URL" "database_api.get_dynamic_global_properties")
  check_result "get_dynamic_global_properties returns valid block_number" "$NODE_NAME" "$RESULT" "jq -e '.result.head_block_number > 0'"

  # Test 2: get_block (block 1)
  RESULT=$(rpc_call "$NODE_URL" "block_api.get_block" '{"block_num":1}')
  check_result "block_api.get_block(1) returns block_id" "$NODE_NAME" "$RESULT" "jq -e '.result.block.block_id | length > 0'"

  # Test 3: get_block (latest)
  HEAD_BLOCK=$(rpc_call "$NODE_URL" "database_api.get_dynamic_global_properties" | jq '.result.head_block_number')
  if [ "$HEAD_BLOCK" != "null" ] && [ "$HEAD_BLOCK" -gt 0 ]; then
    RESULT=$(rpc_call "$NODE_URL" "block_api.get_block" "{\"block_num\":$HEAD_BLOCK}")
    check_result "block_api.get_block($HEAD_BLOCK) returns block_id" "$NODE_NAME" "$RESULT" "jq -e '.result.block.block_id | length > 0'"
  else
    echo "  SKIP: [$NODE_NAME] get_block(latest) - head_block_number unavailable"
  fi

  # Test 4: get_config
  RESULT=$(rpc_call "$NODE_URL" "database_api.get_config")
  check_result "get_config returns STEEM_BLOCKCHAIN_VERSION" "$NODE_NAME" "$RESULT" "jq -e '.result.STEEM_BLOCKCHAIN_VERSION | length > 0'"

  # Test 5: get_hardfork_properties
  RESULT=$(rpc_call "$NODE_URL" "database_api.get_hardfork_properties")
  check_result "get_hardfork_properties returns current_hardfork_version" "$NODE_NAME" "$RESULT" "jq -e '.result.current_hardfork_version | length > 0'"

  # Test 6: get_witness_schedule
  RESULT=$(rpc_call "$NODE_URL" "database_api.get_witness_schedule")
  check_result "get_witness_schedule returns current_shares" "$NODE_NAME" "$RESULT" "jq -e '.result.current_shares | length > 0'"

  # Test 7: condenser_api.get_accounts
  RESULT=$(rpc_call "$NODE_URL" "condenser_api.get_accounts" '[["steemit"]]')
  check_result "condenser_api.get_accounts returns steemit" "$NODE_NAME" "$RESULT" "jq -e '.result[0].name == \"steemit\"'"

  # Test 8: condenser_api.get_block (block 1)
  RESULT=$(rpc_call "$NODE_URL" "condenser_api.get_block" '{"block_num":1}')
  check_result "condenser_api.get_block(1) returns previous" "$NODE_NAME" "$RESULT" "jq -e '.result.previous | length > 0'"

  # Test 9: get_reward_funds
  RESULT=$(rpc_call "$NODE_URL" "database_api.get_reward_funds")
  check_result "get_reward_funds returns non-empty result" "$NODE_NAME" "$RESULT" "jq -e '.result | length > 0'"

  # Test 10: rc_api.get_rc_accounts
  RESULT=$(rpc_call "$NODE_URL" "rc_api.get_rc_accounts" '{"accounts":["steemit"]}')
  check_result "rc_api.get_rc_accounts returns rc_accounts" "$NODE_NAME" "$RESULT" "jq -e '.result.rc_accounts | length > 0'"

  echo ""
done

# Cross-node consistency check
echo "--- Cross-Node Consistency Check ---"
for BLOCK_NUM in 1 100000 1000000 10000000 50000000; do
  ID_A=$(rpc_call "$NODE_A" "block_api.get_block" "{\"block_num\":$BLOCK_NUM}" | jq -r '.result.block.block_id // "UNAVAILABLE"')
  ID_B=$(rpc_call "$NODE_B" "block_api.get_block" "{\"block_num\":$BLOCK_NUM}" | jq -r '.result.block.block_id // "UNAVAILABLE"')

  if [ "$ID_A" = "UNAVAILABLE" ] || [ "$ID_B" = "UNAVAILABLE" ]; then
    echo "  SKIP: block $BLOCK_NUM - one or both nodes unavailable"
  elif [ "$ID_A" = "$ID_B" ]; then
    echo "  MATCH: block $BLOCK_NUM = $ID_A"
    ((PASS++))
  else
    echo "  MISMATCH: block $BLOCK_NUM A=$ID_A B=$ID_B"
    ((FAIL++))
  fi
done

echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "Finished: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo "========================================"

if [ "$FAIL" -gt 0 ]; then
  exit 1
fi
