#pragma once

#include <fc/optional.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain { namespace util { namespace smt {

typedef std::pair< time_point_sec, const smt_token_emissions_object* > emission_data;

const smt_token_object* find_token( const database& db, uint32_t nai );
const smt_token_object* find_token( const database& db, asset_symbol_type symbol, bool precision_agnostic = false );
const smt_token_emissions_object* last_emission( const database& db, const asset_symbol_type& symbol );
fc::optional< time_point_sec > last_emission_time( const database& db, const asset_symbol_type& symbol );
fc::optional< time_point_sec > next_emission_time( const database& db, const asset_symbol_type& symbol, time_point_sec time = time_point_sec() );

namespace ico {

bool schedule_next_refund( database& db, const asset_symbol_type& a );
bool schedule_next_contributor_payout( database& db, const asset_symbol_type& a );
bool schedule_founder_payout( database& db, const asset_symbol_type& a );

share_type payout( database& db, const asset_symbol_type& symbol, const account_object& account, const std::vector< asset >& assets );

} // steem::chain::util::smt::ico

} } } } // steem::chain::util::smt
