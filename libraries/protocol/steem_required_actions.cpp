#include <steem/protocol/validation.hpp>
#include <steem/protocol/steem_required_actions.hpp>

namespace steem { namespace protocol {

#ifdef IS_TEST_NET
void example_required_action::validate()const
{
   validate_account_name( account );
}

bool operator==( const example_required_action& lhs, const example_required_action& rhs )
{
   return lhs.account == rhs.account;
}
#endif

#ifdef STEEM_ENABLE_SMT
void smt_ico_launch_action::validate() const
{
   validate_smt_symbol( symbol );
}

bool operator==( const smt_ico_launch_action& lhs, const smt_ico_launch_action& rhs )
{
   return lhs.symbol == rhs.symbol;
}

void smt_ico_evaluation_action::validate() const
{
   validate_smt_symbol( symbol );
}

bool operator==( const smt_ico_evaluation_action& lhs, const smt_ico_evaluation_action& rhs )
{
   return lhs.symbol == rhs.symbol;
}

void smt_token_launch_action::validate() const
{
   validate_smt_symbol( symbol );
}

bool operator==( const smt_token_launch_action& lhs, const smt_token_launch_action& rhs )
{
   return lhs.symbol == rhs.symbol;
}

void smt_refund_action::validate() const
{
   validate_account_name( contributor );
   validate_smt_symbol( symbol );
}

bool operator==( const smt_refund_action& lhs, const smt_refund_action& rhs )
{
   return
      lhs.symbol == rhs.symbol &&
      lhs.contributor == rhs.contributor &&
      lhs.contribution_id == rhs.contribution_id &&
      lhs.refund == rhs.refund;
}

void smt_contributor_payout_action::validate() const
{
   validate_account_name( contributor );
   validate_smt_symbol( symbol );
}

bool operator==( const smt_contributor_payout_action& lhs, const smt_contributor_payout_action& rhs )
{
   return
      lhs.symbol == rhs.symbol &&
      lhs.contributor == rhs.contributor &&
      lhs.contribution_id == rhs.contribution_id &&
      lhs.payouts == rhs.payouts;
}

void smt_founder_payout_action::validate() const
{
   validate_smt_symbol( symbol );

   for ( auto& payout : payouts )
      validate_account_name( payout.first );
}

bool operator==( const smt_founder_payout_action& lhs, const smt_founder_payout_action& rhs )
{
   return
      lhs.symbol == rhs.symbol &&
      lhs.payouts == rhs.payouts;
}
#endif

} } //steem::protocol
