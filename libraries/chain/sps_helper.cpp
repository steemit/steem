#include <steem/chain/sps_helper.hpp>
#include <steem/chain/sps_objects.hpp>

namespace steem { namespace chain {

template< bool IS_CHECK >
void sps_helper::remove_proposals_impl( database& db, const flat_set<int64_t>& proposal_ids, const account_name_type& proposal_owner )
{
   if( proposal_ids.empty() )
      return;

   auto& proposalIdx = db.get_mutable_index< proposal_index >();
   auto& byIdIdx = proposalIdx.indices().get< by_id >();

   auto& votesIndex = db.get_mutable_index< proposal_vote_index >();
   auto& byVoterIdx = votesIndex.indices().get< by_proposal_voter >();

   for(auto id : proposal_ids)
   {
      auto foundPosI = byIdIdx.find(id);

      if(foundPosI == byIdIdx.end())
         continue;

      const auto& proposal = *foundPosI;

      if( IS_CHECK )
         FC_ASSERT(proposal.creator == proposal_owner, "Only proposal owner can remove it...");

      ilog("Erasing all votes associated to proposal: ${p}", ("p", proposal));

      /// Now remove all votes specific to given proposal.
      auto propI = byVoterIdx.lower_bound(boost::make_tuple(proposal.id, account_name_type()));

      while(propI != byVoterIdx.end() && propI->id == proposal.id)
      {
         propI = votesIndex.erase<by_proposal_voter>(propI);
      }

      ilog("Erasing proposal: ${p}", ("p", proposal));

      proposalIdx.remove(proposal);
   }

}

void sps_helper::remove_proposals( database& db, const flat_set<int64_t>& proposal_ids, const account_name_type& proposal_owner )
{
   remove_proposals_impl< true/*IS_CHECK*/ >( db, proposal_ids, proposal_owner );
}

void sps_helper::remove_proposals( database& db, const flat_set<int64_t>& proposal_ids )
{
   remove_proposals_impl< false/*IS_CHECK*/ >( db, proposal_ids, account_name_type() );
}

} } // steem::chain
