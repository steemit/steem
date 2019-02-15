#!/bin/bash

OBJECTS=( "account_authority"                \
          "account_history"                  \
          "account_metadata"                 \
          "account"                          \
          "account_recovery_request"         \
          "block_summary"                    \
          "change_recovery_account_request"  \
          "comment_content"                  \
          "comment"                          \
          "comment_vote"                     \
          "convert_request"                  \
          "decline_voting_rights_request"    \
          "dynamic_global_property"          \
          "escrow"                           \
          "feed_history"                     \
          "hardfork_property"                \
          "limit_order"                      \
          "liquidity_reward_balance"         \
          "operation"                        \
          "owner_authority_history"          \
          "pending_optional_action"          \
          "pending_required_action"          \
          "reward_fund"                      \
          "savings_withdraw"                 \
          "transaction"                      \
          "vesting_delegation_expiration"    \
          "vesting_delegation"               \
          "withdraw_vesting_route"           \
          "witness"                          \
          "witness_schedule"                 \
          "witness_vote" )


DATA_DIR="$HOME/.steemd"
STEEMD_DIR="../.."
STATS_DUMP_PERIOD=20

ADVISOR_PATH="../../libraries/vendor/rocksdb/tools/advisor"

while (( "$#" )); do
   case "$1" in
      -d|--data-dir)
         DATA_DIR=$2
         shift 2
         ;;
      -s|--steemd-dir)
         STEEMD_DIR=$2
         shift 2
         ;;
      -p|--stats-dump-period)
         STATS_DUMP_PERIOD=$2
         shift 2
         ;;
      -h|--help)
         echo "Specify data directory with '--data-dir' (Default is ~/.steemd)"
         echo "Specify steemd directory with '--steemd-dir' (Default is ../..)"
         echo "Specify stats dump period with '--stats-dump-period' (Default is 20)"
         exit 1
         ;;
      *)
         echo "Specify data directory with '--data-dir' (Default is ~/.steemd)"
         echo "Specify steemd directory with '--steemd-dir' (Default is ../..)"
         echo "Specify stats dump period with '--stats-dump-period' (Default is 20)"
         exit 1
         ;;
   esac
done

cd "$STEEMD_DIR/libraries/vendor/rocksdb/tools/advisor"

for OBJ in "${OBJECTS[@]}"; do
   DB_PATH="$DATA_DIR/blockchain/rocksdb_"
   DB_PATH+="$OBJ"
   DB_PATH+='_object'

   echo "Advisor for $OBJ..."
   python3 -m advisor.rule_parser_example --rules_spec=advisor/rules.ini --rocksdb_options="$DB_PATH/OPTIONS-000014" --log_files_path_prefix="$DB_PATH/LOG" --stats_dump_period_sec=$STATS_DUMP_PERIOD
   echo ''
done

exit 0
