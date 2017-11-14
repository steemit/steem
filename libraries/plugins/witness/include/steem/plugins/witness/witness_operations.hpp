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

struct egg_pow_input
{
   fc::sha256                 h_egg_auth;
   protocol::block_id_type    recent_block_id;
   fc::uint128_t              nonce;

   uint32_t calculate_pow_summary()const;
};

struct egg_pow
{
   egg_pow_input              input;
   uint32_t                   pow_summary = 0;

   void validate()const;
};

struct create_egg_operation : base_operation
{
   void validate()const;

   void get_required_authorities( vector< protocol::authority >& auths )const
   {   auths.emplace_back( egg_auth );   }

   egg_pow                    work;
   protocol::authority        egg_auth;
};

typedef fc::static_variant<
         enable_content_editing_operation,
         create_egg_operation
      > witness_plugin_operation;

STEEM_DEFINE_PLUGIN_EVALUATOR( witness_plugin, witness_plugin_operation, enable_content_editing );
STEEM_DEFINE_PLUGIN_EVALUATOR( witness_plugin, witness_plugin_operation, create_egg );

} } } // steem::plugins::witness

FC_REFLECT( steem::plugins::witness::enable_content_editing_operation, (account)(relock_time) )

FC_REFLECT( steem::plugins::witness::egg_pow_input, (h_egg_auth)(recent_block_id)(nonce) )
FC_REFLECT( steem::plugins::witness::egg_pow, (input)(pow_summary) )
FC_REFLECT( steem::plugins::witness::create_egg_operation, (work)(egg_auth) )

FC_REFLECT_TYPENAME( steem::plugins::witness::witness_plugin_operation )

STEEM_DECLARE_OPERATION_TYPE( steem::plugins::witness::witness_plugin_operation )
