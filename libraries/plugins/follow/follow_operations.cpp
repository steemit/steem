#include <steemit/plugins/follow/follow_operations.hpp>

#include <steemit/protocol/operation_util_impl.hpp>

namespace steemit { namespace plugins{ namespace follow {

void follow_operation::validate()const
{
   FC_ASSERT( follower != following, "You cannot follow yourself" );
}

void reblog_operation::validate()const
{
   FC_ASSERT( account != author, "You cannot reblog your own content" );
}

} } } //steemit::plugins::follow

STEEM_DEFINE_OPERATION_TYPE( steemit::plugins::follow::follow_plugin_operation )
