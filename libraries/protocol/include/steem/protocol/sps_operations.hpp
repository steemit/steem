#pragma once
#include <steem/protocol/base.hpp>

#include <steem/protocol/asset.hpp>


namespace steem { namespace protocol {

struct create_proposal_operation : public base_operation
{
   account_name_type creator;
   /// Account to be paid if given proposal has sufficient votes.
   account_name_type receiver;

   time_point_sec start_date;
   time_point_sec end_date;

   /// Amount of SBDs to be daily paid to the `receiver` account.
   asset daily_pay;

   string subject;

   /// Given url shall be a valid permlink.
   string url;

   void validate()const;

   void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( creator ); }
};

struct update_proposal_votes_operation : public base_operation
{
   account_name_type voter;

   /// IDs of proposals to vote for/against. Nonexisting IDs are ignored.
   flat_set<int64_t> proposal_ids;

   bool approve = false;

   void validate()const;

   void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( voter ); }
};

/** Allows to remove proposals specified by given IDs.
    Operation can be performed only by proposal owner. 
*/
struct remove_proposal_operation : public base_operation
{
   account_name_type proposal_owner;

   /// IDs of proposals to be removed. Nonexisting IDs are ignored.
   flat_set<int64_t> proposal_ids;

   void validate() const;

   void get_required_active_authorities(flat_set<account_name_type>& a)const { a.insert(proposal_owner); }
};

} } // steem::protocol

FC_REFLECT( steem::protocol::create_proposal_operation, (creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(url) )
FC_REFLECT( steem::protocol::update_proposal_votes_operation, (voter)(proposal_ids)(approve) )
FC_REFLECT( steem::protocol::remove_proposal_operation, (proposal_owner)(proposal_ids) )

