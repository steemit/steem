/*
 * Copyright (c) 2016 Steemit, Inc., and contributors.
 */
#pragma once

#ifdef IS_TEST_NET
#define STEEMIT_INIT_PRIVATE_KEY                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define STEEMIT_INIT_PUBLIC_KEY_STR             (std::string( steemit::chain::public_key_type(STEEMIT_INIT_PRIVATE_KEY.get_public_key()) ))
#define STEEMIT_CHAIN_ID                        (fc::sha256::hash("testnet"))

#define VESTS_SYMBOL  (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< VESTS with 6 digits of precision
#define STEEM_SYMBOL  (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< STEEM with 3 digits of precision
#define SBD_SYMBOL    (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('B') << 16) | (uint64_t('D') << 24) ) ///< Test Backed Dollars with 3 digits of precision

#define STEEMIT_SYMBOL                          "TEST"
#define DEFAULT_ASSET_SYMBOL                    STEEM_SYMBOL
#define STEEMIT_ADDRESS_PREFIX                  "TST"

#define STEEMIT_GENESIS_TIME                    (fc::time_point_sec())
#define STEEMIT_MINING_TIME                     (fc::time_point_sec())
#define STEEMIT_FIRST_CASHOUT_TIME              (fc::time_point_sec())
#define STEEMIT_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr

#define STEEMIT_MIN_ACCOUNT_CREATION_FEE        0


#else // IS LIVE STEEM NETWORK

#define STEEMIT_INIT_PUBLIC_KEY_STR             "STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"
#define STEEMIT_CHAIN_ID                        (steemit::chain::chain_id_type())
#define VESTS_SYMBOL  (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< VESTS with 6 digits of precision
#define STEEM_SYMBOL  (uint64_t(3) | (uint64_t('S') << 8) | (uint64_t('T') << 16) | (uint64_t('E') << 24) | (uint64_t('E') << 32) | (uint64_t('M') << 40)) ///< STEEM with 3 digits of precision
#define SBD_SYMBOL    (uint64_t(3) | (uint64_t('S') << 8) | (uint64_t('B') << 16) | (uint64_t('D') << 24) ) ///< STEEM Backed Dollars with 3 digits of precision
#define STEEMIT_SYMBOL                          "STEEM"
#define DEFAULT_ASSET_SYMBOL                    STEEM_SYMBOL
#define STEEMIT_ADDRESS_PREFIX                  "STM"

#define STEEMIT_GENESIS_TIME                    (fc::time_point_sec(1458835200))
#define STEEMIT_MINING_TIME                     (fc::time_point_sec(1458838800))
#define STEEMIT_FIRST_CASHOUT_TIME              (fc::time_point_sec(1467590400))  /// July 4th
#define STEEMIT_CASHOUT_WINDOW_SECONDS          (60*60*24)  /// 1 day

#define STEEMIT_MIN_ACCOUNT_CREATION_FEE        100000

#endif


#define STEEMIT_BLOCK_INTERVAL                  3
#define STEEMIT_BLOCKS_PER_YEAR                 (365*24*60*60/STEEMIT_BLOCK_INTERVAL)
#define STEEMIT_BLOCKS_PER_DAY                  (24*60*60/STEEMIT_BLOCK_INTERVAL)
#define STEEMIT_START_VESTING_BLOCK             (STEEMIT_BLOCKS_PER_DAY * 7)
#define STEEMIT_START_MINER_VOTING_BLOCK        (STEEMIT_BLOCKS_PER_DAY * 30)

#define STEEMIT_INIT_MINER_NAME                 "initminer"
#define STEEMIT_NUM_INIT_MINERS                 1
#define STEEMIT_INIT_TIME                       (fc::time_point_sec());
#define STEEMIT_MAX_MINERS                      21 /// 21 is more than enough
#define STEEMIT_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define STEEMIT_MAX_MEMO_SIZE                   2048
#define STEEMIT_MAX_PROXY_RECURSION_DEPTH       4
#define STEEMIT_VESTING_WITHDRAW_INTERVALS      104
#define STEEMIT_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) /// 1 week per interval
#define STEEMIT_VOTE_REGENERATION_SECONDS       (60*60*24) // 1 day

#define STEEMIT_100_PERCENT                     10000
#define STEEMIT_1_PERCENT                       (STEEMIT_100_PERCENT/100)
#define STEEMIT_DEFAULT_SBD_INTEREST_RATE       (10*STEEMIT_1_PERCENT) ///< 10% APR

#define STEEMIT_MINER_PAY_PERCENT               (STEEMIT_1_PERCENT) // 1%
#define STEEMIT_MIN_RATION                      100000
#define STEEMIT_MAX_RATION_DECAY_RATE           (1000000)
#define STEEMIT_FREE_TRANSACTIONS_WITH_NEW_ACCOUNT 100

#define STEEMIT_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define STEEMIT_BANDWIDTH_PRECISION             1000000ll ///< 1 million
#define STEEMIT_MAX_COMMENT_DEPTH               6

#define STEEMIT_MAX_RESERVE_RATIO   (10000)


#define STEEMIT_MINING_REWARD                   asset( 1000, STEEM_SYMBOL )

#define STEEMIT_LIQUIDITY_TIMEOUT_SEC           (fc::seconds(60*60*24*7)) // After one week volume is set to 0
#define STEEMIT_MIN_LIQUIDITY_REWARD_PERIOD_SEC (fc::seconds(60)) // 1 minute required on books to receive volume
#define STEEMIT_LIQUIDITY_REWARD_PERIOD_SEC     (60*60)
#define STEEMIT_LIQUIDITY_REWARD_BLOCKS         (STEEMIT_LIQUIDITY_REWARD_PERIOD_SEC/STEEMIT_BLOCK_INTERVAL)
#define STEEMIT_MIN_LIQUIDITY_REWARD            (asset( 1000*STEEMIT_LIQUIDITY_REWARD_BLOCKS, STEEM_SYMBOL )) // Minumum reward to be paid out to liquidity providers
#define STEEMIT_MIN_CONTENT_REWARD              STEEMIT_MINING_REWARD
#define STEEMIT_MIN_CURATE_REWARD               STEEMIT_MINING_REWARD
#define STEEMIT_MIN_PRODUCER_REWARD             STEEMIT_MINING_REWARD
#define STEEMIT_MIN_POW_REWARD                  STEEMIT_MINING_REWARD

#define STEEMIT_CURATE_APR                      5
#define STEEMIT_CONTENT_APR                     5
#define STEEMIT_LIQUIDITY_APR                   1
#define STEEMIT_PRODUCER_APR                    1
#define STEEMIT_POW_APR                         1

#define STEEMIT_MIN_PAYOUT_SBD                  (asset(20,SBD_SYMBOL))

#define STEEMIT_MIN_ACCOUNT_NAME_LENGTH          3
#define STEEMIT_MAX_ACCOUNT_NAME_LENGTH         16

#define STEEMIT_INIT_SUPPLY                     int64_t(0)
#define STEEMIT_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)
#define STEEMIT_MAX_SIG_CHECK_DEPTH             2

#define STEEMIT_MIN_TRANSACTION_SIZE_LIMIT      1024
#define STEEMIT_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define STEEMIT_SBD_INTEREST_COMPOUND_INTERVAL_SEC  (60*60*24*30)
#define STEEMIT_MAX_TRANSACTION_SIZE            (1024*128)
#define STEEMIT_MIN_BLOCK_SIZE_LIMIT            (STEEMIT_MAX_TRANSACTION_SIZE)
#define STEEMIT_MAX_BLOCK_SIZE                  (STEEMIT_MAX_TRANSACTION_SIZE*STEEMIT_BLOCK_INTERVAL*2000)
#define STEEMIT_BLOCKS_PER_HOUR                 (60*60/STEEMIT_BLOCK_INTERVAL)
#define STEEMIT_FEED_INTERVAL_BLOCKS            (STEEMIT_BLOCKS_PER_HOUR)
#define STEEMIT_FEED_HISTORY_WINDOW             (24*7) /// 7 days * 24 hours per day
#define STEEMIT_MAX_FEED_AGE                    (fc::days(7))
#define STEEMIT_MIN_FEEDS                       (STEEMIT_MAX_MINERS/3) /// protects the network from conversions before price has been established
#define STEEMIT_CONVERSION_DELAY                (fc::days(7))

#define STEEMIT_MIN_UNDO_HISTORY                10
#define STEEMIT_MAX_UNDO_HISTORY                10000

#define STEEMIT_MIN_TRANSACTION_EXPIRATION_LIMIT (STEEMIT_BLOCK_INTERVAL * 5) // 5 transactions per block
#define STEEMIT_BLOCKCHAIN_PRECISION            uint64_t( 1000 )

#define STEEMIT_BLOCKCHAIN_PRECISION_DIGITS     3
//#define STEEMIT_TRANSFER_FEE                    (1*STEEMIT_BLOCKCHAIN_PRECISION)
#define STEEMIT_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define STEEMIT_MAX_AUTHORITY_MEMBERSHIP        10
#define STEEMIT_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define STEEMIT_MAX_URL_LENGTH                  127

// counter initialization values used to derive near and far future seeds for shuffling witnesses
// we use the fractional bits of sqrt(2) in hex
#define STEEMIT_NEAR_SCHEDULE_CTR_IV            ( (uint64_t( 0x6a09 ) << 0x30)    \
                                                 | (uint64_t( 0xe667 ) << 0x20)    \
                                                 | (uint64_t( 0xf3bc ) << 0x10)    \
                                                 | (uint64_t( 0xc908 )        ) )

// and the fractional bits of sqrt(3) in hex
#define STEEMIT_FAR_SCHEDULE_CTR_IV             ( (uint64_t( 0xbb67 ) << 0x30)    \
                                                 | (uint64_t( 0xae85 ) << 0x20)    \
                                                 | (uint64_t( 0x84ca ) << 0x10)    \
                                                 | (uint64_t( 0xa73b )        ) )

/**
 * every second, the fraction of burned core asset which cycles is
 * STEEMIT_CORE_ASSET_CYCLE_RATE / (1 << STEEMIT_CORE_ASSET_CYCLE_RATE_BITS)
 */
#define STEEMIT_CORE_ASSET_CYCLE_RATE           17
#define STEEMIT_CORE_ASSET_CYCLE_RATE_BITS      32

#define GRAPHENE_CURRENT_DB_VERSION             "GPH2.4"

#define STEEMIT_IRREVERSIBLE_THRESHOLD          (51 * STEEMIT_1_PERCENT)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current witnesses
#define STEEMIT_MINER_ACCOUNT                   "miners"
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define STEEMIT_NULL_ACCOUNT                    "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define STEEMIT_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define STEEMIT_PROXY_TO_SELF_ACCOUNT           ""
///@}
