#include <steem/protocol/validation.hpp>
#include <steem/protocol/steem_optional_actions.hpp>

namespace steem { namespace protocol {

void example_optional_action::validate()const
{
   validate_account_name( account );
}

} } //steem::protocol
