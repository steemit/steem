#pragma once
#include <string>
#include <steem/protocol/types.hpp>

#ifdef STEEM_ENABLE_SMT

#define SMT_DESTINATION_PREFIX         "$"
#define SMT_DESTINATION_VESTING_SUFFIX ".vesting"
#define SMT_DESTINATION_FROM           unit_target_type( SMT_DESTINATION_PREFIX "from" )
#define SMT_DESTINATION_FROM_VESTING   unit_target_type( SMT_DESTINATION_PREFIX "from" SMT_DESTINATION_VESTING_SUFFIX )
#define SMT_DESTINATION_MARKET_MAKER   unit_target_type( SMT_DESTINATION_PREFIX "market_maker" )
#define SMT_DESTINATION_REWARDS        unit_target_type( SMT_DESTINATION_PREFIX "rewards" )
#define SMT_DESTINATION_VESTING        unit_target_type( SMT_DESTINATION_PREFIX "vesting" )

namespace steem { namespace protocol { namespace utilities { namespace smt {

namespace unit_target {

bool is_contributor( const unit_target_type& unit_target );
bool is_market_maker( const unit_target_type& unit_target );
bool is_rewards( const unit_target_type& unit_target );
bool is_founder_vesting( const unit_target_type& unit_target );
bool is_vesting( const unit_target_type& unit_target );

bool is_account_name_type( const unit_target_type& unit_target );
bool is_vesting_type( const unit_target_type& unit_target );

account_name_type get_unit_target_account( const unit_target_type& unit_target );

} // steem::protocol::utilities::smt::unit_target

} } } } // steem::protocol::utilities::smt

#endif
