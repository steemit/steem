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
void smt_event_action::validate() const
{
   validate_smt_symbol( symbol );

   switch ( event )
   {
      case smt_event::ico_launch:
      case smt_event::ico_evaluation:
      case smt_event::token_launch:
         break;
      default:
         FC_ASSERT( false, "Invalid SMT event processing submission" );
   }
}

bool operator==( const smt_event_action& lhs, const smt_event_action& rhs )
{
   return
      lhs.symbol == rhs.symbol &&
      lhs.event == rhs.event;
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
      lhs.contribution_id == rhs.contribution_id;
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
      lhs.contribution_id == rhs.contribution_id;
}
#endif

} } //steem::protocol
