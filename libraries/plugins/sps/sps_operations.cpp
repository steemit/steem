#include <steem/plugins/sps/sps_operations.hpp>

#include <steem/protocol/operation_util_impl.hpp>

namespace steem { namespace plugins{ namespace sps {

void create_proposal_operation::validate()const
{
}

void update_proposal_votes_operation::validate()const
{
}

} } } //steem::plugins::sps

STEEM_DEFINE_OPERATION_TYPE( steem::plugins::sps::sps_plugin_operation )
