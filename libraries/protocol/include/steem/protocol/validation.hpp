
#pragma once

#include <steem/protocol/base.hpp>
#include <steem/protocol/block_header.hpp>
#include <steem/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace steem { namespace protocol {

inline bool is_asset_type( asset asset, asset_symbol_type symbol )
{
   return asset.symbol == symbol;
}

inline void validate_account_name( const string& name )
{
   FC_ASSERT( is_valid_account_name( name ), "Account name ${n} is invalid", ("n", name) );
}

inline void validate_permlink( const string& permlink )
{
   FC_ASSERT( permlink.size() < STEEM_MAX_PERMLINK_LENGTH, "permlink is too long" );
   FC_ASSERT( fc::is_utf8( permlink ), "permlink not formatted in UTF8" );
}

inline void validate_smt_symbol( const asset_symbol_type& symbol )
{
   symbol.validate();
   FC_ASSERT( symbol.space() == asset_symbol_type::smt_nai_space, "legacy symbol used instead of NAI" );
   FC_ASSERT( symbol.is_vesting() == false, "liquid variant of NAI expected");
}

/**
 * Validate a price complies with Tick Pricing Rules.
 *
 * - For prices involving Steem Dollars (SBD), the base asset must be SBD.
 * - For prices involving SMT assets, the base asset must be STEEM.
 * - The quote must be a power of 10.
 *
 * \param price The price to perform Tick Pricing Rules validation
 * \throw fc::assert_exception If the price does not conform to Tick Pricing Rules
 */
inline void validate_tick_pricing( const price& p )
{
   if ( p.base.symbol == SBD_SYMBOL )
   {
      FC_ASSERT( p.quote.symbol == STEEM_SYMBOL, "Only STEEM can be paired with SBD as the base symbol." );
   }
   else if ( p.base.symbol == STEEM_SYMBOL )
   {
      FC_ASSERT( p.quote.symbol.space() == asset_symbol_type::smt_nai_space, "Only tokens can be paired with STEEM as the base symbol." );
   }
   else
   {
      FC_ASSERT( false, "STEEM and SBD are the only valid base symbols." );
   }

   share_type tmp = p.quote.amount;
   while ( tmp > 9 && tmp % 10 == 0 )
      tmp /= 10;
   FC_ASSERT( tmp == 1, "The quote amount must be a power of 10." );
}

} }
