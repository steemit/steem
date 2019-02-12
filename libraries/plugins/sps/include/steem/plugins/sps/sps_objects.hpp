#pragma once
#include <steem/chain/steem_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace steem { namespace plugins { namespace sps {

using namespace std;
using namespace steem::chain;

#ifndef STEEM_SPS_SPACE_ID
#define STEEM_SPS_SPACE_ID 20
#endif

enum sps_plugin_object_type
{
   proposal_object_type       = ( STEEM_SPS_SPACE_ID << 8 ),
   proposal_vote_object_type  = ( STEEM_SPS_SPACE_ID << 8 ) + 1
};

class proposal_object : public object< proposal_object_type, proposal_object >
{
   proposal_object() = delete;

   public:

      template<typename Constructor, typename Allocator>
      proposal_object( Constructor&& c, allocator< Allocator > a )
      : subject( a ), url( a )
      {
         c(*this);
      };

      //internal key
      id_type id;

      // account that created the proposal
      account_name_type creator;

      // account_being_funded
      account_name_type receiver;

      // start_date (when the proposal will begin paying out if it gets enough vote weight)
      time_point_sec start_date;

      // end_date (when the proposal expires and can no longer pay out)
      time_point_sec end_date;

      //daily_pay (the amount of SBD that is being requested to be paid out daily)
      asset daily_pay;

      //subject (a very brief description or title for the proposal)
      shared_string subject;

      //url (a link to a page describing the work proposal in depth, generally this will probably be to a Steem post).
      shared_string url;

      //This will be calculate every maintenance period
      uint64_t total_votes = 0;
};

typedef oid< proposal_object > proposal_id_type;

class proposal_vote_object : public object< proposal_vote_object_type, proposal_vote_object>
{
   proposal_vote_object() = delete;

   public:

      template< typename Constructor, typename Allocator >
      proposal_vote_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      //internal key
      id_type id;

      //account that made voting
      account_name_type voter;

      //the voter voted for this proposal number
      proposal_id_type proposal_id;
};

typedef oid< proposal_vote_object > proposal_vote_id_type;

struct by_date;
struct by_creator;
struct by_total_votes;

using namespace boost::multi_index;

typedef multi_index_container<
   proposal_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< proposal_object, proposal_id_type, &proposal_object::id > >,
      ordered_non_unique< tag< by_date >,
         composite_key< proposal_object,
            member< proposal_object, time_point_sec, &proposal_object::start_date >,
            member< proposal_object, time_point_sec, &proposal_object::end_date >
            >
       >,
      ordered_non_unique< tag< by_creator >, member< proposal_object, account_name_type, &proposal_object::creator > >,
      ordered_non_unique< tag< by_total_votes >, member< proposal_object, uint64_t, &proposal_object::total_votes > >
   >,
   allocator< proposal_object >
> proposal_index;

struct by_voter_proposal;

typedef multi_index_container<
   proposal_vote_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< proposal_vote_object, proposal_vote_id_type, &proposal_vote_object::id > >,
      ordered_unique< tag< by_voter_proposal >,
         composite_key< proposal_vote_object,
            member< proposal_vote_object, account_name_type, &proposal_vote_object::voter >,
            member< proposal_vote_object, proposal_id_type, &proposal_vote_object::proposal_id >
            >
       >
   >,
   allocator< proposal_vote_object >
> proposal_vote_index;

} } } // steem::plugins::sps

FC_REFLECT( steem::plugins::sps::proposal_object, (id)(creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(url)(total_votes) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::sps::proposal_object, steem::plugins::sps::proposal_index )

FC_REFLECT( steem::plugins::sps::proposal_vote_object, (id)(voter)(proposal_id) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::sps::proposal_vote_object, steem::plugins::sps::proposal_vote_index )
