#include <steem/plugins/sps/sps_plugin.hpp>
#include <steem/plugins/sps/sps_operations.hpp>
#include <steem/plugins/sps/sps_objects.hpp>


namespace steem { namespace plugins { namespace sps {

void create_proposal_evaluator::do_apply( const create_proposal_operation& o )
{
   try
   {
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void update_proposal_votes_evaluator::do_apply( const update_proposal_votes_operation& o )
{
   try
   {
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}


} } } // steem::plugins::sps
