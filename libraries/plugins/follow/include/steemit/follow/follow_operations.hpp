#pragma once

#include <steemit/chain/protocol/base.hpp>

#include <steemit/follow/follow_plugin.hpp>

namespace steemit { namespace follow {

using namespace std;

struct follow_operation : base_operation
{
    aname_type              follower;
    aname_type              following;
    set< string >       what; /// blog, mute

    void validate()const;

    void get_required_posting_authorities( flat_set<aname_type>& a )const { a.insert( follower ); }
};

struct reblog_operation : base_operation
{
   aname_type account;
   aname_type author;
   string permlink;

   void validate()const;

   void get_required_posting_authorities( flat_set<aname_type>& a )const { a.insert( account ); }
};

typedef fc::static_variant<
         follow_operation,
         reblog_operation
      > follow_plugin_operation;

DEFINE_PLUGIN_EVALUATOR( follow_plugin, follow_plugin_operation, follow );
DEFINE_PLUGIN_EVALUATOR( follow_plugin, follow_plugin_operation, reblog );

} } // steemit::follow

FC_REFLECT( steemit::follow::follow_operation, (follower)(following)(what) )
FC_REFLECT( steemit::follow::reblog_operation, (account)(author)(permlink) )

DECLARE_OPERATION_TYPE( steemit::follow::follow_plugin_operation )

FC_REFLECT_TYPENAME( steemit::follow::follow_plugin_operation )
