#include <steemit/witness/witness_operations.hpp>

#include <steemit/protocol/operation_util_impl.hpp>

namespace steemit { namespace witness {

void enable_content_editing_operation::validate()const
{
   chain::validate_account_name( account );
}

} } // steemit::witness

DEFINE_OPERATION_TYPE( steemit::witness::witness_plugin_operation )
