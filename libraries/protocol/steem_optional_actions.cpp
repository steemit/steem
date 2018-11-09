#include <steem/protocol/validation.hpp>
#include <steem/protocol/steem_optional_actions.hpp>

namespace steem { namespace protocol {

#ifdef IS_TEST_NET
void example_optional_action::validate()const
{
   validate_account_name( account );
}
#endif

} } //steem::protocol
