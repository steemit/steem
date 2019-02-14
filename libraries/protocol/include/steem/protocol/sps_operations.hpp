#pragma once
#include <steem/protocol/base.hpp>

#include <steem/protocol/asset.hpp>


namespace steem { namespace protocol {

struct create_proposal_operation : base_operation
{
   account_name_type creator;
   account_name_type receiver;

   time_point_sec start_date;
   time_point_sec end_date;

   asset daily_pay;

   std::string subject;

   std::string url;

   void validate()const;

   void get_required_posting_authorities( flat_set<account_name_type>& a )const { a.insert( creator ); }
};

struct update_proposal_votes_operation : base_operation
{
   account_name_type voter;

   std::vector<int64_t> proposal_ids;

   bool approve;

   void validate()const;

   void get_required_posting_authorities( flat_set<account_name_type>& a )const { a.insert( voter ); }
};

} } // steem::protocol

FC_REFLECT( steem::protocol::create_proposal_operation, (creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(url) )
FC_REFLECT( steem::protocol::update_proposal_votes_operation, (voter)(proposal_ids)(approve) )
