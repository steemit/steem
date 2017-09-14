#pragma once

#include <steem/protocol/base.hpp>
#include <steem/protocol/operation_util.hpp>

#include <steem/chain/evaluator.hpp>

namespace steem { namespace plugins { namespace witness {

using namespace std;
using steem::protocol::base_operation;
using steem::chain::database;

class witness_plugin;

struct enable_content_editing_operation : base_operation
{
   protocol::account_name_type   account;
   fc::time_point_sec            relock_time;

   void validate()const;

   void get_required_active_authorities( flat_set< protocol::account_name_type>& a )const { a.insert( account ); }
};

typedef fc::static_variant<
         enable_content_editing_operation
      > witness_plugin_operation;

STEEM_DEFINE_PLUGIN_EVALUATOR( witness_plugin, witness_plugin_operation, enable_content_editing );

} } } // steem::plugins::witness

FC_REFLECT( steem::plugins::witness::enable_content_editing_operation, (account)(relock_time) )

FC_REFLECT_TYPENAME( steem::plugins::witness::witness_plugin_operation )

STEEM_DECLARE_OPERATION_TYPE( steem::plugins::witness::witness_plugin_operation )
