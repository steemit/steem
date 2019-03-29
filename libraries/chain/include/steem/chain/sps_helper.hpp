#pragma once

#include <steem/chain/database.hpp>
#include <steem/chain/sps_objects.hpp>

#include <boost/container/flat_set.hpp>

namespace steem { namespace chain {

using boost::container::flat_set;

class sps_helper
{
   public:

      template<   typename ByProposalType,
                  typename ProposalObjectIterator,
                  typename ProposalIndex, typename VotesIndex, typename ByVoterIdx >
      static ProposalObjectIterator remove_proposal( const ProposalObjectIterator& proposal,
                  ProposalIndex& proposalIndex, VotesIndex& votesIndex, const ByVoterIdx& byVoterIdx )
      {
         ilog("Erasing all votes associated to proposal: ${p}", ("p", *proposal));

         /// Now remove all votes specific to given proposal.
         auto propI = byVoterIdx.lower_bound(boost::make_tuple(proposal->id, account_name_type()));

         while(propI != byVoterIdx.end() && static_cast< size_t >( propI->proposal_id ) == static_cast< size_t >( proposal->id ) )
         {
            propI = votesIndex. template erase<by_proposal_voter>(propI);
         }

         ilog("Erasing proposal: ${p}", ("p", *proposal));

         return proposalIndex. template erase< ByProposalType >( proposal );
      }

      static void remove_proposals( database& db, const flat_set<int64_t>& proposal_ids, const account_name_type& proposal_owner );
};

} } // steem::chain
