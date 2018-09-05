#include <steem/protocol/validation.hpp>
#include <steem/protocol/steem_required_actions.hpp>

namespace steem { namespace protocol {

void required_action::validate()const
{
   validate_account_name( account );
}

bool operator==( const required_action& lhs, const required_action& rhs )
{
   return lhs.account == rhs.account;
}

} } //steem::protocol
