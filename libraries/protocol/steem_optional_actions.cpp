#include <steem/protocol/validation.hpp>
#include <steem/protocol/steem_optional_actions.hpp>

namespace steem { namespace protocol {

#ifdef IS_TEST_NET
void example_optional_action::validate()const
{
   validate_account_name( account );
}
#endif

void smt_token_emission_action::validate() const
{
   validate_account_name( control_account );
   validate_smt_symbol( symbol );
}

} } //steem::protocol
