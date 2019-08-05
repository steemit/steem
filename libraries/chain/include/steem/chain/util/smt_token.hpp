#pragma once

#ifdef STEEM_ENABLE_SMT

#include <fc/optional.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain { namespace util { namespace smt {

const smt_token_object* find_token( const database& db, uint32_t nai );
const smt_token_object* find_token( const database& db, asset_symbol_type symbol, bool precision_agnostic = false );
const smt_token_emissions_object* last_emission( const database& db, const asset_symbol_type& symbol );
fc::optional< time_point_sec > last_emission_time( const database& db, const asset_symbol_type& symbol );

namespace ico {

bool schedule_next_refund( database& db, const asset_symbol_type& a );
bool schedule_next_contributor_payout( database& db, const asset_symbol_type& a );
bool schedule_founder_payout( database& db, const asset_symbol_type& a );
void payout( database& db, const asset_symbol_type& symbol, const account_object& account, const std::vector< asset >& assets );

} // steem::chain::util::smt::ico

namespace generation_unit {

account_name_type get_unit_target_account( const unit_target_type& unit_target );
bool is_contributor( const unit_target_type& unit_target );
bool is_market_maker( const unit_target_type& unit_target );
bool is_rewards( const unit_target_type& unit_target );
bool is_vesting( const unit_target_type& unit_target );
bool is_founder_vesting( const unit_target_type& unit_target );

} // steem::chain::util::smt::generation_unit

} } } } // steem::chain::util::smt

#endif
