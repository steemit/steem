/*
 * Copyright (c) 2016 Dpnit, Inc., and contributors.
 */
#pragma once
#include <dpn/protocol/hardfork.hpp>

// WARNING!
// Every symbol defined here needs to be handled appropriately in get_config.cpp
// This is checked by get_config_check.sh called from Dockerfile

#ifdef IS_TEST_NET
#define DPN_BLOCKCHAIN_VERSION              ( version(0, 22, 0) )

#define DPN_INIT_PRIVATE_KEY                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define DPN_INIT_PUBLIC_KEY_STR             (std::string( dpn::protocol::public_key_type(DPN_INIT_PRIVATE_KEY.get_public_key()) ))
#define DPN_CHAIN_ID (fc::sha256::hash("testnet"))
#define DPN_ADDRESS_PREFIX                  "TST"

#define DPN_GENESIS_TIME                    (fc::time_point_sec(1451606400))
#define DPN_MINING_TIME                     (fc::time_point_sec(1451606400))
#define DPN_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr
#define DPN_CASHOUT_WINDOW_SECONDS_PRE_HF12 (DPN_CASHOUT_WINDOW_SECONDS)
#define DPN_CASHOUT_WINDOW_SECONDS_PRE_HF17 (DPN_CASHOUT_WINDOW_SECONDS)
#define DPN_SECOND_CASHOUT_WINDOW           (60*60*24*3) /// 3 days
#define DPN_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24) /// 1 day
#define DPN_UPVOTE_LOCKOUT_HF7              (fc::minutes(1))
#define DPN_UPVOTE_LOCKOUT_SECONDS          (60*5)    /// 5 minutes
#define DPN_UPVOTE_LOCKOUT_HF17             (fc::minutes(5))


#define DPN_MIN_ACCOUNT_CREATION_FEE          0
#define DPN_MAX_ACCOUNT_CREATION_FEE          int64_t(1000000000)

#define DPN_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define DPN_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define DPN_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define DPN_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#define DPN_INIT_SUPPLY                     (int64_t( 250 ) * int64_t( 1000000 ) * int64_t( 1000 ))
#define DPN_DBD_INIT_SUPPLY                 (int64_t( 7 ) * int64_t( 1000000 ) * int64_t( 1000 ))

/// Allows to limit number of total produced blocks.
#define TESTNET_BLOCK_LIMIT                   (3000000)

#else // IS LIVE DPN NETWORK

#define DPN_BLOCKCHAIN_VERSION              ( version(0, 21, 0) )

#define DPN_INIT_PUBLIC_KEY_STR             "STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"
#define DPN_CHAIN_ID fc::sha256()
#define DPN_ADDRESS_PREFIX                  "DPN"

#define DPN_GENESIS_TIME                    (fc::time_point_sec(1458835200))
#define DPN_MINING_TIME                     (fc::time_point_sec(1458838800))
#define DPN_CASHOUT_WINDOW_SECONDS_PRE_HF12 (60*60*24)    /// 1 day
#define DPN_CASHOUT_WINDOW_SECONDS_PRE_HF17 (60*60*12)    /// 12 hours
#define DPN_CASHOUT_WINDOW_SECONDS          (60*60*24*7)  /// 7 days
#define DPN_SECOND_CASHOUT_WINDOW           (60*60*24*30) /// 30 days
#define DPN_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24*14) /// 2 weeks
#define DPN_UPVOTE_LOCKOUT_HF7              (fc::minutes(1))
#define DPN_UPVOTE_LOCKOUT_SECONDS          (60*60*12)    /// 12 hours
#define DPN_UPVOTE_LOCKOUT_HF17             (fc::hours(12))

#define DPN_MIN_ACCOUNT_CREATION_FEE           1
#define DPN_MAX_ACCOUNT_CREATION_FEE           int64_t(1000000000)

#define DPN_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define DPN_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define DPN_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define DPN_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 3186477

#define DPN_INIT_SUPPLY                     int64_t(0)
#define DPN_DBD_INIT_SUPPLY                 int64_t(0)

#endif

#define VESTS_SYMBOL  (dpn::protocol::asset_symbol_type::from_asset_num( DPN_ASSET_NUM_VESTS ) )
#define DPN_SYMBOL  (dpn::protocol::asset_symbol_type::from_asset_num( DPN_ASSET_NUM_DPN ) )
#define DBD_SYMBOL    (dpn::protocol::asset_symbol_type::from_asset_num( DPN_ASSET_NUM_DBD ) )

#define DPN_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( DPN_BLOCKCHAIN_VERSION ) )

#define DPN_100_PERCENT                     10000
#define DPN_1_PERCENT                       (DPN_100_PERCENT/100)

#define DPN_BLOCK_INTERVAL                  0.5
#define DPN_BLOCKS_PER_YEAR                 (365*24*60*60/DPN_BLOCK_INTERVAL)
#define DPN_BLOCKS_PER_DAY                  (24*60*60/DPN_BLOCK_INTERVAL)
#define DPN_START_VESTING_BLOCK             (DPN_BLOCKS_PER_DAY * 7)
#define DPN_START_MINER_VOTING_BLOCK        (DPN_BLOCKS_PER_DAY * 30)

#define DPN_INIT_MINER_NAME                 "initminer"
#define DPN_NUM_INIT_MINERS                 1
#define DPN_INIT_TIME                       (fc::time_point_sec());

#define DPN_MAX_WITNESSES                   63

#define DPN_MAX_VOTED_WITNESSES_HF0         19
#define DPN_MAX_MINER_WITNESSES_HF0         1
#define DPN_MAX_RUNNER_WITNESSES_HF0        1

#define DPN_MAX_VOTED_WITNESSES_HF17        20
#define DPN_MAX_MINER_WITNESSES_HF17        0
#define DPN_MAX_RUNNER_WITNESSES_HF17       1

#define DPN_HARDFORK_REQUIRED_WITNESSES     1 // 17 of the 21 dpos witnesses (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define DPN_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define DPN_MAX_MEMO_SIZE                   2048
#define DPN_MAX_PROXY_RECURSION_DEPTH       4
#define DPN_VESTING_WITHDRAW_INTERVALS_PRE_HF_16 104
#define DPN_VESTING_WITHDRAW_INTERVALS      13
#define DPN_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) /// 1 week per interval
#define DPN_MAX_WITHDRAW_ROUTES             10
#define DPN_SAVINGS_WITHDRAW_TIME        	(fc::days(3))
#define DPN_SAVINGS_WITHDRAW_REQUEST_LIMIT  100
#define DPN_VOTING_MANA_REGENERATION_SECONDS (5*60*60*24) // 5 day
#define DPN_MAX_VOTE_CHANGES                5
#define DPN_REVERSE_AUCTION_WINDOW_SECONDS_HF6 (60*30) /// 30 minutes
#define DPN_REVERSE_AUCTION_WINDOW_SECONDS_HF20 (60*15) /// 15 minutes
#define DPN_MIN_VOTE_INTERVAL_SEC           3
#define DPN_VOTE_DUST_THRESHOLD             (50000000)
#define DPN_DOWNVOTE_POOL_PERCENT_HF21      (25*DPN_1_PERCENT)

#define DPN_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define DPN_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds
#define DPN_MIN_REPLY_INTERVAL_HF20         (fc::seconds(3)) // 3 seconds
#define DPN_MIN_COMMENT_EDIT_INTERVAL       (fc::seconds(3)) // 3 seconds
#define DPN_POST_AVERAGE_WINDOW             (60*60*24u) // 1 day
#define DPN_POST_WEIGHT_CONSTANT            (uint64_t(4*DPN_100_PERCENT) * (4*DPN_100_PERCENT))// (4*DPN_100_PERCENT) -> 2 posts per 1 days, average 1 every 12 hours

#define DPN_MAX_ACCOUNT_WITNESS_VOTES       15

#define DPN_DEFAULT_DBD_INTEREST_RATE       (10*DPN_1_PERCENT) ///< 10% APR

#define DPN_INFLATION_RATE_START_PERCENT    (978) // Fixes block 7,000,000 to 9.5%
#define DPN_INFLATION_RATE_STOP_PERCENT     (95) // 0.95%
#define DPN_INFLATION_NARROWING_PERIOD      (250000) // Narrow 0.01% every 250k blocks
#define DPN_CONTENT_REWARD_PERCENT_HF16     (75*DPN_1_PERCENT) //75% of inflation, 7.125% inflation
#define DPN_VESTING_FUND_PERCENT_HF16       (15*DPN_1_PERCENT) //15% of inflation, 1.425% inflation
#define DPN_PROPOSAL_FUND_PERCENT_HF0       (0)

#define DPN_CONTENT_REWARD_PERCENT_HF21     (0*DPN_1_PERCENT)
#define DPN_PROPOSAL_FUND_PERCENT_HF21      (75*DPN_1_PERCENT)

#define DPN_HF21_CONVERGENT_LINEAR_RECENT_CLAIMS (fc::uint128_t(0,503600561838938636ull))
#define DPN_CONTENT_CONSTANT_HF21           (fc::uint128_t(0,2000000000000ull))

#define DPN_MINER_PAY_PERCENT               (DPN_1_PERCENT) // 1%
#define DPN_MAX_RATION_DECAY_RATE           (1000000)

#define DPN_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define DPN_BANDWIDTH_PRECISION             (uint64_t(1000000)) ///< 1 million
#define DPN_MAX_COMMENT_DEPTH_PRE_HF17      6
#define DPN_MAX_COMMENT_DEPTH               0xffff // 64k
#define DPN_SOFT_MAX_COMMENT_DEPTH          0xff // 255

#define DPN_MAX_RESERVE_RATIO               (20000)

#define DPN_CREATE_ACCOUNT_WITH_DPN_MODIFIER 30
#define DPN_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define DPN_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)

#define DPN_MINING_REWARD                   asset( 1000, DPN_SYMBOL )
#define DPN_EQUIHASH_N                      140
#define DPN_EQUIHASH_K                      6

#define DPN_LIQUIDITY_TIMEOUT_SEC           (fc::seconds(60*60*24*7)) // After one week volume is set to 0
#define DPN_MIN_LIQUIDITY_REWARD_PERIOD_SEC (fc::seconds(60)) // 1 minute required on books to receive volume
#define DPN_LIQUIDITY_REWARD_PERIOD_SEC     (60*60)
#define DPN_LIQUIDITY_REWARD_BLOCKS         (DPN_LIQUIDITY_REWARD_PERIOD_SEC/DPN_BLOCK_INTERVAL)
#define DPN_MIN_LIQUIDITY_REWARD            (asset( 1000*DPN_LIQUIDITY_REWARD_BLOCKS, DPN_SYMBOL )) // Minumum reward to be paid out to liquidity providers
#define DPN_MIN_CONTENT_REWARD              DPN_MINING_REWARD
#define DPN_MIN_CURATE_REWARD               DPN_MINING_REWARD
#define DPN_MIN_PRODUCER_REWARD             DPN_MINING_REWARD
#define DPN_MIN_POW_REWARD                  DPN_MINING_REWARD

#define DPN_ACTIVE_CHALLENGE_FEE            asset( 2000, DPN_SYMBOL )
#define DPN_OWNER_CHALLENGE_FEE             asset( 30000, DPN_SYMBOL )
#define DPN_ACTIVE_CHALLENGE_COOLDOWN       fc::days(1)
#define DPN_OWNER_CHALLENGE_COOLDOWN        fc::days(1)

#define DPN_POST_REWARD_FUND_NAME           ("post")
#define DPN_COMMENT_REWARD_FUND_NAME        ("comment")
#define DPN_RECENT_RSHARES_DECAY_TIME_HF17    (fc::days(30))
#define DPN_RECENT_RSHARES_DECAY_TIME_HF19    (fc::days(15))
#define DPN_CONTENT_CONSTANT_HF0            (uint128_t(uint64_t(2000000000000ll)))
// note, if redefining these constants make sure calculate_claims doesn't overflow

// 5ccc e802 de5f
// int(expm1( log1p( 1 ) / BLOCKS_PER_YEAR ) * 2**DPN_APR_PERCENT_SHIFT_PER_BLOCK / 100000 + 0.5)
// we use 100000 here instead of 10000 because we end up creating an additional 9x for vesting
#define DPN_APR_PERCENT_MULTIPLY_PER_BLOCK          ( (uint64_t( 0x5ccc ) << 0x20) \
                                                        | (uint64_t( 0xe802 ) << 0x10) \
                                                        | (uint64_t( 0xde5f )        ) \
                                                        )
// chosen to be the maximal value such that DPN_APR_PERCENT_MULTIPLY_PER_BLOCK * 2**64 * 100000 < 2**128
#define DPN_APR_PERCENT_SHIFT_PER_BLOCK             87

#define DPN_APR_PERCENT_MULTIPLY_PER_ROUND          ( (uint64_t( 0x79cc ) << 0x20 ) \
                                                        | (uint64_t( 0xf5c7 ) << 0x10 ) \
                                                        | (uint64_t( 0x3480 )         ) \
                                                        )

#define DPN_APR_PERCENT_SHIFT_PER_ROUND             83

// We have different constants for hourly rewards
// i.e. hex(int(math.expm1( math.log1p( 1 ) / HOURS_PER_YEAR ) * 2**DPN_APR_PERCENT_SHIFT_PER_HOUR / 100000 + 0.5))
#define DPN_APR_PERCENT_MULTIPLY_PER_HOUR           ( (uint64_t( 0x6cc1 ) << 0x20) \
                                                        | (uint64_t( 0x39a1 ) << 0x10) \
                                                        | (uint64_t( 0x5cbd )        ) \
                                                        )

// chosen to be the maximal value such that DPN_APR_PERCENT_MULTIPLY_PER_HOUR * 2**64 * 100000 < 2**128
#define DPN_APR_PERCENT_SHIFT_PER_HOUR              77

// These constants add up to GRAPHENE_100_PERCENT.  Each GRAPHENE_1_PERCENT is equivalent to 1% per year APY
// *including the corresponding 9x vesting rewards*
#define DPN_CURATE_APR_PERCENT              3875
#define DPN_CONTENT_APR_PERCENT             3875
#define DPN_LIQUIDITY_APR_PERCENT            750
#define DPN_PRODUCER_APR_PERCENT             750
#define DPN_POW_APR_PERCENT                  750

#define DPN_MIN_PAYOUT_DBD                  (asset(20,DBD_SYMBOL))

#define DPN_DBD_STOP_PERCENT_HF14           (5*DPN_1_PERCENT ) // Stop printing DBD at 5% Market Cap
#define DPN_DBD_STOP_PERCENT_HF20           (10*DPN_1_PERCENT ) // Stop printing DBD at 10% Market Cap
#define DPN_DBD_START_PERCENT_HF14          (2*DPN_1_PERCENT) // Start reducing printing of DBD at 2% Market Cap
#define DPN_DBD_START_PERCENT_HF20          (9*DPN_1_PERCENT) // Start reducing printing of DBD at 9% Market Cap
#define DPN_DBD_STOP_ADJUST                  5 // Make a small adjustment to the stop percent to stop at the debt limit (see issue 3184)

#define DPN_MIN_ACCOUNT_NAME_LENGTH          3
#define DPN_MAX_ACCOUNT_NAME_LENGTH         16

#define DPN_MIN_PERMLINK_LENGTH             0
#define DPN_MAX_PERMLINK_LENGTH             256
#define DPN_MAX_WITNESS_URL_LENGTH          2048

#define DPN_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)
#define DPN_MAX_SATOSHIS                    int64_t(4611686018427387903ll)
#define DPN_MAX_SIG_CHECK_DEPTH             2
#define DPN_MAX_SIG_CHECK_ACCOUNTS          125

#define DPN_MIN_TRANSACTION_SIZE_LIMIT      1024
#define DPN_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define DPN_DBD_INTEREST_COMPOUND_INTERVAL_SEC  (60*60*24*30)
#define DPN_MAX_TRANSACTION_SIZE            (1024*64)
#define DPN_MIN_BLOCK_SIZE_LIMIT            (DPN_MAX_TRANSACTION_SIZE)
#define DPN_MAX_BLOCK_SIZE                  (DPN_MAX_TRANSACTION_SIZE*DPN_BLOCK_INTERVAL*2000)
#define DPN_SOFT_MAX_BLOCK_SIZE             (2*1024*1024)
#define DPN_MIN_BLOCK_SIZE                  115
#define DPN_BLOCKS_PER_HOUR                 (60*60/DPN_BLOCK_INTERVAL)
#define DPN_FEED_INTERVAL_BLOCKS            (DPN_BLOCKS_PER_HOUR)
#define DPN_FEED_HISTORY_WINDOW_PRE_HF_16   (24*7) /// 7 days * 24 hours per day
#define DPN_FEED_HISTORY_WINDOW             (12*7) // 3.5 days
#define DPN_MAX_FEED_AGE_SECONDS            (60*60*24*7) // 7 days
#define DPN_MIN_FEEDS                       (DPN_MAX_WITNESSES/3) /// protects the network from conversions before price has been established
#define DPN_CONVERSION_DELAY_PRE_HF_16      (fc::days(7))
#define DPN_CONVERSION_DELAY                (fc::hours(DPN_FEED_HISTORY_WINDOW)) //3.5 day conversion

#define DPN_MIN_UNDO_HISTORY                10
#define DPN_MAX_UNDO_HISTORY                10000

#define DPN_MIN_TRANSACTION_EXPIRATION_LIMIT (DPN_BLOCK_INTERVAL * 5) // 5 transactions per block
#define DPN_BLOCKCHAIN_PRECISION            uint64_t( 1000 )

#define DPN_BLOCKCHAIN_PRECISION_DIGITS     3
#define DPN_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define DPN_MAX_AUTHORITY_MEMBERSHIP        40
#define DPN_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define DPN_MAX_URL_LENGTH                  127

#define DPN_IRREVERSIBLE_THRESHOLD          (75 * DPN_1_PERCENT)

#define DPN_VIRTUAL_SCHEDULE_LAP_LENGTH  ( fc::uint128(uint64_t(-1)) )
#define DPN_VIRTUAL_SCHEDULE_LAP_LENGTH2 ( fc::uint128::max_value() )

#define DPN_INITIAL_VOTE_POWER_RATE (40)
#define DPN_REDUCED_VOTE_POWER_RATE (10)

#define DPN_MAX_LIMIT_ORDER_EXPIRATION     (60*60*24*28) // 28 days
#define DPN_DELEGATION_RETURN_PERIOD_HF0   (DPN_CASHOUT_WINDOW_SECONDS)
#define DPN_DELEGATION_RETURN_PERIOD_HF20  (DPN_VOTING_MANA_REGENERATION_SECONDS)

#define DPN_RD_MIN_DECAY_BITS               6
#define DPN_RD_MAX_DECAY_BITS              32
#define DPN_RD_DECAY_DENOM_SHIFT           36
#define DPN_RD_MAX_POOL_BITS               64
#define DPN_RD_MAX_BUDGET_1                ((uint64_t(1) << (DPN_RD_MAX_POOL_BITS + DPN_RD_MIN_DECAY_BITS - DPN_RD_DECAY_DENOM_SHIFT))-1)
#define DPN_RD_MAX_BUDGET_2                ((uint64_t(1) << (64-DPN_RD_DECAY_DENOM_SHIFT))-1)
#define DPN_RD_MAX_BUDGET_3                (uint64_t( std::numeric_limits<int32_t>::max() ))
#define DPN_RD_MAX_BUDGET                  (int32_t( std::min( { DPN_RD_MAX_BUDGET_1, DPN_RD_MAX_BUDGET_2, DPN_RD_MAX_BUDGET_3 } )) )
#define DPN_RD_MIN_DECAY                   (uint32_t(1) << DPN_RD_MIN_DECAY_BITS)
#define DPN_RD_MIN_BUDGET                  1
#define DPN_RD_MAX_DECAY                   (uint32_t(0xFFFFFFFF))

#define DPN_ACCOUNT_SUBSIDY_PRECISION      (DPN_100_PERCENT)

// We want the global subsidy to run out first in normal (Poisson)
// conditions, so we boost the per-witness subsidy a little.
#define DPN_WITNESS_SUBSIDY_BUDGET_PERCENT (125 * DPN_1_PERCENT)

// Since witness decay only procs once per round, multiplying the decay
// constant by the number of witnesses means the per-witness pools have
// the same effective decay rate in real-time terms.
#define DPN_WITNESS_SUBSIDY_DECAY_PERCENT  (DPN_MAX_WITNESSES * DPN_100_PERCENT)

// 347321 corresponds to a 5-day halflife
#define DPN_DEFAULT_ACCOUNT_SUBSIDY_DECAY  (347321)
// Default rate is 0.5 accounts per block
#define DPN_DEFAULT_ACCOUNT_SUBSIDY_BUDGET (797)
#define DPN_DECAY_BACKSTOP_PERCENT         (90 * DPN_1_PERCENT)

#define DPN_BLOCK_GENERATION_POSTPONED_TX_LIMIT 5
#define DPN_PENDING_TRANSACTION_EXECUTION_LIMIT fc::milliseconds(200)

#define DPN_CUSTOM_OP_ID_MAX_LENGTH        (32)
#define DPN_CUSTOM_OP_DATA_MAX_LENGTH      (8192)
#define DPN_BENEFICIARY_LIMIT              (128)
#define DPN_COMMENT_TITLE_LIMIT            (256)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current witnesses
#define DPN_MINER_ACCOUNT                   "miners"
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define DPN_NULL_ACCOUNT                    "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define DPN_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define DPN_PROXY_TO_SELF_ACCOUNT           ""
/// Represents the canonical root post parent account
#define DPN_ROOT_POST_PARENT                (account_name_type())
/// Represents the account with NO authority which holds resources for payouts according to given proposals
#define DPN_TREASURY_ACCOUNT                "dpn.dao"
///@}

/// DPN PROPOSAL SYSTEM support

#define DPN_TREASURY_FEE                         (10 * DPN_BLOCKCHAIN_PRECISION)
#define DPN_PROPOSAL_MAINTENANCE_PERIOD          3600
#define DPN_PROPOSAL_MAINTENANCE_CLEANUP         (60*60*24*1) /// 1 day
#define DPN_PROPOSAL_SUBJECT_MAX_LENGTH          80
/// Max number of IDs passed at once to the update_proposal_voter_operation or remove_proposal_operation.
#define DPN_PROPOSAL_MAX_IDS_NUMBER              5

#ifdef DPN_ENABLE_SMT

#define SMT_MAX_VOTABLE_ASSETS 2
#define SMT_VESTING_WITHDRAW_INTERVAL_SECONDS   (60*60*24*7) /// 1 week per interval
#define SMT_UPVOTE_LOCKOUT                      (60*60*12)   /// 12 hours
#define SMT_EMISSION_MIN_INTERVAL_SECONDS       (60*60*6)    /// 6 hours
#define SMT_EMIT_INDEFINITELY                   (std::numeric_limits<uint32_t>::max())
#define SMT_MAX_NOMINAL_VOTES_PER_DAY           (1000)
#define SMT_MAX_VOTES_PER_REGENERATION          ((SMT_MAX_NOMINAL_VOTES_PER_DAY * SMT_VESTING_WITHDRAW_INTERVAL_SECONDS) / 86400)
#define SMT_DEFAULT_VOTES_PER_REGEN_PERIOD      (50)
#define SMT_DEFAULT_PERCENT_CURATION_REWARDS    (25 * DPN_1_PERCENT)

#endif /// DPN_ENABLE_SMT

