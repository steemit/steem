#include <dpn/plugins/follow/follow_operations.hpp>

#include <dpn/protocol/operation_util_impl.hpp>

namespace dpn { namespace plugins{ namespace follow {

void follow_operation::validate()const
{
   FC_ASSERT( follower != following, "You cannot follow yourself" );
}

void reblog_operation::validate()const
{
   FC_ASSERT( account != author, "You cannot reblog your own content" );
}

} } } //dpn::plugins::follow

DPN_DEFINE_OPERATION_TYPE( dpn::plugins::follow::follow_plugin_operation )
