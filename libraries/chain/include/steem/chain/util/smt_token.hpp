#pragma once

#ifdef STEEM_ENABLE_SMT

#include <fc/optional.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain { namespace util {

const smt_token_object* find_smt_token( database& db, uint32_t nai );
const smt_token_object* find_smt_token( database& db, asset_symbol_type symbol, bool precision_agnostic = false );
time_point_sec calculate_schedule_end_time( const time_point_sec& schedule_time, uint32_t interval_seconds, uint32_t interval_count );
fc::optional< time_point_sec > smt_token_emissions_end_time( database& db, const asset_symbol_type& symbol );

} } } // steem::chain::util

#endif
