#pragma once
#include <steem/protocol/base.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/evaluator.hpp>

#include <steem/protocol/asset.hpp>


namespace steem { namespace plugins { namespace sps {

using steem::protocol::account_name_type;
using steem::protocol::base_operation;
using steem::protocol::asset;

using namespace steem::chain;

class sps_plugin;

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

typedef fc::static_variant<
         create_proposal_operation,
         update_proposal_votes_operation
      > sps_plugin_operation;

STEEM_DEFINE_PLUGIN_EVALUATOR( sps_plugin, sps_plugin_operation, create_proposal );
STEEM_DEFINE_PLUGIN_EVALUATOR( sps_plugin, sps_plugin_operation, update_proposal_votes );

} } } // steem::plugins::sps

FC_REFLECT( steem::plugins::sps::create_proposal_operation, (creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(url) )
FC_REFLECT( steem::plugins::sps::update_proposal_votes_operation, (voter)(proposal_ids)(approve) )

STEEM_DECLARE_OPERATION_TYPE( steem::plugins::sps::sps_plugin_operation )

FC_REFLECT_TYPENAME( steem::plugins::sps::sps_plugin_operation )
