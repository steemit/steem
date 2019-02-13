#include <steem/plugins/sps/sps_operations.hpp>

#include <steem/protocol/operation_util_impl.hpp>

namespace steem { namespace plugins{ namespace sps {

void create_proposal_operation::validate()const
{
   validate_account_name( creator );
   validate_account_name( receiver );

   FC_ASSERT( end_date > start_date, "end date must be greater than start date" );

   FC_ASSERT( !subject.empty(), "subject is required" );
   FC_ASSERT( !url.empty(), "url is required" );
}

void update_proposal_votes_operation::validate()const
{
   validate_account_name( voter );
}

} } } //steem::plugins::sps

STEEM_DEFINE_OPERATION_TYPE( steem::plugins::sps::sps_plugin_operation )
