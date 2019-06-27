#include <dpn/protocol/validation.hpp>
#include <dpn/protocol/dpn_optional_actions.hpp>

namespace dpn { namespace protocol {

#ifdef IS_TEST_NET
void example_optional_action::validate()const
{
   validate_account_name( account );
}
#endif

} } //dpn::protocol
