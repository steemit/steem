#include <steemit/plugins/witness/witness_operations.hpp>

#include <steemit/protocol/operation_util_impl.hpp>

namespace steemit { namespace plugins { namespace witness {

void enable_content_editing_operation::validate()const
{
   protocol::validate_account_name( account );
}

} } } // steemit::plugins::witness

DEFINE_OPERATION_TYPE( steemit::plugins::witness::witness_plugin_operation )
