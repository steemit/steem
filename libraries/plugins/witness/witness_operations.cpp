#include <steemit/plugins/witness/witness_operations.hpp>

#include <steemit/protocol/operation_util_impl.hpp>

namespace steem { namespace plugins { namespace witness {

void enable_content_editing_operation::validate()const
{
   protocol::validate_account_name( account );
}

} } } // steem::plugins::witness

STEEM_DEFINE_OPERATION_TYPE( steem::plugins::witness::witness_plugin_operation )
