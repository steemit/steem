#include <steem/protocol/smt_util.hpp>
#include <steem/protocol/authority.hpp>

namespace steem { namespace protocol { namespace smt {

namespace unit_target {

bool is_contributor( const unit_target_type& unit_target )
{
   return unit_target == SMT_DESTINATION_FROM || unit_target == SMT_DESTINATION_FROM_VESTING;
}

bool is_market_maker( const unit_target_type& unit_target )
{
   return unit_target == SMT_DESTINATION_MARKET_MAKER;
}

bool is_rewards( const unit_target_type& unit_target )
{
   return unit_target == SMT_DESTINATION_REWARDS;
}

bool is_founder_vesting( const unit_target_type& unit_target )
{
   std::string unit_target_str = unit_target;
   if ( unit_target_str.size() > std::strlen( SMT_DESTINATION_ACCOUNT_PREFIX ) + std::strlen( SMT_DESTINATION_VESTING_SUFFIX ) )
   {
      auto pos = unit_target_str.find( SMT_DESTINATION_ACCOUNT_PREFIX );
      if ( pos != std::string::npos && pos == 0 )
      {
         std::size_t suffix_len = std::strlen( SMT_DESTINATION_VESTING_SUFFIX );
         if ( !unit_target_str.compare( unit_target_str.size() - suffix_len, suffix_len, SMT_DESTINATION_VESTING_SUFFIX ) )
         {
            return true;
         }
      }
   }

   return false;
}

bool is_vesting( const unit_target_type& unit_target )
{
   return unit_target == SMT_DESTINATION_VESTING;
}

bool is_account_name_type( const unit_target_type& unit_target )
{
   if ( is_contributor( unit_target ) )
      return false;
   if ( is_rewards( unit_target ) )
      return false;
   if ( is_market_maker( unit_target ) )
      return false;
   if ( is_vesting( unit_target ) )
      return false;
   return true;
}

bool is_vesting_type( const unit_target_type& unit_target )
{
   if ( unit_target == SMT_DESTINATION_FROM_VESTING )
      return true;
   if ( unit_target == SMT_DESTINATION_VESTING )
      return true;
   if ( is_founder_vesting( unit_target ) )
      return true;

   return false;
}

account_name_type get_unit_target_account( const unit_target_type& unit_target )
{
   FC_ASSERT( !is_contributor( unit_target ),  "Cannot derive an account name from a contributor special destination." );
   FC_ASSERT( !is_market_maker( unit_target ), "The market maker unit target is not a valid account." );
   FC_ASSERT( !is_rewards( unit_target ),      "The rewards unit target is not a valid account." );

   if ( is_valid_account_name( unit_target ) )
      return account_name_type( unit_target );

   // This is a special unit target destination in the form of $alice.vesting
   FC_ASSERT( unit_target.size() > std::strlen( SMT_DESTINATION_ACCOUNT_PREFIX ) + std::strlen( SMT_DESTINATION_VESTING_SUFFIX ),
      "Unit target '${target}' is malformed", ("target", unit_target) );

   std::string str_name = unit_target;
   auto pos = str_name.find( SMT_DESTINATION_ACCOUNT_PREFIX );
   FC_ASSERT( pos != std::string::npos && pos == 0, "Expected SMT destination account prefix '${prefix}' for unit target '${target}'.",
      ("prefix", SMT_DESTINATION_ACCOUNT_PREFIX)("target", unit_target) );

   std::size_t suffix_len = std::strlen( SMT_DESTINATION_VESTING_SUFFIX );
   FC_ASSERT( !str_name.compare( str_name.size() - suffix_len, suffix_len, SMT_DESTINATION_VESTING_SUFFIX ),
      "Expected SMT destination vesting suffix '${suffix}' for unit target '${target}'.",
         ("suffix", SMT_DESTINATION_VESTING_SUFFIX)("target", unit_target) );

   std::size_t prefix_len = std::strlen( SMT_DESTINATION_ACCOUNT_PREFIX );
   account_name_type unit_target_account = str_name.substr( prefix_len, str_name.size() - suffix_len - prefix_len );
   FC_ASSERT( is_valid_account_name( unit_target_account ), "The derived unit target account name '${name}' is invalid.",
      ("name", unit_target_account) );

   return unit_target_account;
}

} // steem::protocol::smt::unit_target

} } } // steem::protocol::smt
