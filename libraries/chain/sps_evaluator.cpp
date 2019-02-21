#include <steem/protocol/sps_operations.hpp>

#include <steem/chain/database.hpp>

#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/sps_objects.hpp>


namespace steem { namespace chain {

using steem::chain::create_proposal_evaluator;

void create_proposal_evaluator::do_apply( const create_proposal_operation& o )
{
   try
   {
      ilog("creating proposal: ${op}", ("op", o));

      asset fee_sbd( STEEM_TREASURY_FEE, SBD_SYMBOL );

      FC_ASSERT( _db.get_balance( o.creator, SBD_SYMBOL ) >= fee_sbd,
         "Account does not have sufficient funds for specified fee of ${of}", ("of", fee_sbd) );

      //only for check if given account exists
      _db.get_account( STEEM_TREASURY_ACCOUNT );

      const auto& owner_account = _db.get_account( o.creator );
      const auto& receiver_account = _db.get_account( o.receiver );

      _db.create< proposal_object >( [&]( proposal_object& proposal )
      {
         proposal.creator = o.creator;
         proposal.receiver = o.receiver;

         proposal.start_date = o.start_date;
         proposal.end_date = o.end_date;

         proposal.daily_pay = o.daily_pay;

         proposal.subject = o.subject.c_str();

         proposal.url = o.url.c_str();
      });

      _db.adjust_balance( owner_account, -fee_sbd );
      _db.adjust_balance( receiver_account, fee_sbd );
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void update_proposal_votes_evaluator::do_apply( const update_proposal_votes_operation& o )
{
   try
   {
      ilog("voting proposal: ${op}", ("op", o));

      if( o.proposal_ids.empty() )
         return;

      const auto& pidx = _db.get_index< proposal_index >().indices().get< by_id >();
      const auto& pvidx = _db.get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      for( const auto id : o.proposal_ids )
      {
         //checking if proposal id exists
         auto found_id = pidx.find( id );
         if( found_id == pidx.end() )
            continue;

         auto found = pvidx.find( boost::make_tuple( o.voter, id ) );

         if( o.approve )
         {
            if( found == pvidx.end() )
               _db.create< proposal_vote_object >( [&]( proposal_vote_object& proposal_vote )
               {
                  proposal_vote.voter = o.voter;
                  proposal_vote.proposal_id = id;
               } );
         }
         else
         {
            if( found != pvidx.end() )
               _db.remove( *found );
         }
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void remove_proposal_evaluator::do_apply(const remove_proposal_operation& op)
{
   try
   {
      ilog("Attempting to evaluate remove_proposal_operation: ${o}", ("o", op));
      
      auto& proposalIdx = _db.get_mutable_index< proposal_index >();
      auto& byIdIdx = proposalIdx.indices().get< by_id >();

      auto& votesIndex = _db.get_mutable_index< proposal_vote_index >();
      auto& byVoterIdx = votesIndex.indices().get< by_proposal_voter >();

      for(auto id : op.proposal_ids)
      {
         auto foundPosI = byIdIdx.find(id);

         if(foundPosI == byIdIdx.end())
            continue;

         const auto& proposal = *foundPosI;

         FC_ASSERT(proposal.creator == op.proposal_owner, "Only proposal owner can remove it...");

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
   FC_CAPTURE_AND_RETHROW( (op) )
}

} } // steem::chain
