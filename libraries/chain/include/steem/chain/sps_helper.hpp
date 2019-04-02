#pragma once

#include <steem/chain/database.hpp>
#include <steem/chain/sps_objects.hpp>

#include <boost/container/flat_set.hpp>

namespace steem { namespace chain {

using boost::container::flat_set;

struct sps_removing_reducer
{
   int16_t threshold;
   int16_t counter = 0;

   bool done = false;

   sps_removing_reducer( int16_t _threshold = -1 )
      : threshold( _threshold )
   {

   }
};

class sps_helper
{
   private:

      template<   typename ByProposalType,
                  bool Loop,
                  typename ProposalObjectIterator,
                  typename ProposalIndex
                  >
      static ProposalObjectIterator checker( const ProposalObjectIterator& proposal, ProposalIndex& proposalIndex, sps_removing_reducer& obj_perf )
      {
         obj_perf.done = false;

         auto end = [&]() -> decltype( proposalIndex.indices(). template get< ByProposalType >().end() )
         {
            return proposalIndex.indices(). template get< ByProposalType >().end();
         };

         auto calc = [&]()
         {
            ++obj_perf.counter;
            obj_perf.done = obj_perf.counter > obj_perf.threshold;
         };

         if( Loop )
         {
            if( obj_perf.threshold < 0 )
               return end();

            calc();

            return end();
         }
         else
         {
            if( obj_perf.threshold < 0 )
               return proposalIndex. template erase< ByProposalType >( proposal );

            calc();

            return obj_perf.done ? end() : proposalIndex. template erase< ByProposalType >( proposal );
         }
      }

   public:

      template<   typename ByProposalType,
                  typename ProposalObjectIterator,
                  typename ProposalIndex, typename VotesIndex, typename ByVoterIdx >
      static ProposalObjectIterator remove_proposal( const ProposalObjectIterator& proposal,
                  ProposalIndex& proposalIndex, VotesIndex& votesIndex, const ByVoterIdx& byVoterIdx, sps_removing_reducer& obj_perf )
      {
         ilog("Erasing all votes associated to proposal: ${p}", ("p", *proposal));

         /// Now remove all votes specific to given proposal.
         auto propI = byVoterIdx.lower_bound(boost::make_tuple(proposal->id, account_name_type()));

         while(propI != byVoterIdx.end() && static_cast< size_t >( propI->proposal_id ) == static_cast< size_t >( proposal->id ) )
         {
            auto result_itr = checker< ByProposalType, true/*Loop*/ >( proposal, proposalIndex, obj_perf );
            if( obj_perf.done )
               return result_itr;

            propI = votesIndex. template erase<by_proposal_voter>(propI);
         }

         ilog("Erasing proposal: ${p}", ("p", *proposal));

         return checker< ByProposalType, false/*Loop*/ >( proposal, proposalIndex, obj_perf );
      }

      static void remove_proposals( database& db, const flat_set<int64_t>& proposal_ids, const account_name_type& proposal_owner );
};

} } // steem::chain
