#pragma once
#include <steem/chain/steem_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <steem/protocol/asset.hpp>

namespace steem { namespace chain {

using steem::chain::object;
using steem::chain::allocator;
using steem::chain::account_name_type;
//using steem::chain::asset;
using steem::protocol::asset;
using steem::chain::shared_string;
using steem::chain::oid;
using steem::chain::by_id;

class proposal_object : public object< proposal_object_type, proposal_object >
{
   proposal_object() = delete;

   public:

      template<typename Constructor, typename Allocator>
      proposal_object( Constructor&& c, allocator< Allocator > a )
      : subject( a ), permlink( a )
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

      //permlink (a link to a page describing the work proposal in depth, generally this will probably be to a Steem post).
      shared_string permlink;

      //This will be calculate every maintenance period
      uint64_t total_votes = 0;

      time_point_sec get_end_date_with_delay() const
      {
         time_point_sec ret = end_date;
         ret += STEEM_PROPOSAL_MAINTENANCE_CLEANUP;

         return ret;
      }
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

struct by_start_date;
struct by_end_date;
struct by_creator;
struct by_total_votes;

typedef multi_index_container<
   proposal_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< proposal_object, proposal_id_type, &proposal_object::id > >,
      ordered_unique< tag< by_start_date >,
         composite_key< proposal_object,
            member< proposal_object, time_point_sec, &proposal_object::start_date >,
            member< proposal_object, proposal_id_type, &proposal_object::id >
         >,
         composite_key_compare< std::less< time_point_sec >, std::less< proposal_id_type > >
      >,
      ordered_unique< tag< by_end_date >,
         composite_key< proposal_object,
            const_mem_fun< proposal_object, time_point_sec, &proposal_object::get_end_date_with_delay >,
            member< proposal_object, proposal_id_type, &proposal_object::id >
         >,
         composite_key_compare< std::less< time_point_sec >, std::less< proposal_id_type > >
      >,
      ordered_unique< tag< by_creator >,
         composite_key< proposal_object,
            member< proposal_object, account_name_type, &proposal_object::creator >,
            member< proposal_object, proposal_id_type, &proposal_object::id >
         >,
         composite_key_compare< std::less< account_name_type >, std::less< proposal_id_type > >
      >,
      ordered_unique< tag< by_total_votes >,
         composite_key< proposal_object,
            member< proposal_object, uint64_t, &proposal_object::total_votes >,
            member< proposal_object, proposal_id_type, &proposal_object::id >
         >,
         composite_key_compare< std::less< uint64_t >, std::less< proposal_id_type > >
      >
   >,
   allocator< proposal_object >
> proposal_index;

struct by_voter_proposal;
struct by_proposal_voter;

typedef multi_index_container<
   proposal_vote_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< proposal_vote_object, proposal_vote_id_type, &proposal_vote_object::id > >,
      ordered_unique< tag< by_voter_proposal >,
         composite_key< proposal_vote_object,
            member< proposal_vote_object, account_name_type, &proposal_vote_object::voter >,
            member< proposal_vote_object, proposal_id_type, &proposal_vote_object::proposal_id >
            >
       >,
      ordered_unique< tag< by_proposal_voter >,
         composite_key< proposal_vote_object,
            member< proposal_vote_object, proposal_id_type, &proposal_vote_object::proposal_id >,
            member< proposal_vote_object, account_name_type, &proposal_vote_object::voter >
            >
       >
   >,
   allocator< proposal_vote_object >
> proposal_vote_index;

} } // steem::chain

FC_REFLECT( steem::chain::proposal_object, (id)(creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(permlink)(total_votes) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::proposal_object, steem::chain::proposal_index )

FC_REFLECT( steem::chain::proposal_vote_object, (id)(voter)(proposal_id) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::proposal_vote_object, steem::chain::proposal_vote_index )
