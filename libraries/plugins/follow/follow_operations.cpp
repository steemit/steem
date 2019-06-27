#include <steem/plugins/follow/follow_operations.hpp>

#include <steem/protocol/operation_util_impl.hpp>

namespace steem { namespace plugins{ namespace follow {

void follow_operation::validate()const
{
   FC_ASSERT( follower != following, "You cannot follow yourself" );
}

void reblog_operation::validate()const
{
   FC_ASSERT( account != author, "You cannot reblog your own content" );
}

} } } //steem::plugins::follow

STEEM_DEFINE_OPERATION_TYPE( steem::plugins::follow::follow_plugin_operation )
