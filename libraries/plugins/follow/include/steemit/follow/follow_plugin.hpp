#pragma once
#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>

#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <fc/thread/future.hpp>
#include <fc/api.hpp>

namespace steemit { namespace follow {
using namespace chain;

#ifndef PRIVATE_MESSAGE_SPACE_ID
#define PRIVATE_MESSAGE_SPACE_ID 8
#endif


enum follow_object_type
{
   key_account_object_type = 0,
   bucket_object_type = 1,///< used in market_history_plugin
   follow_object_type = 6,
   feed_object_type   = 7
};


namespace detail { class follow_plugin_impl; }


class  follow_object : public abstract_object<follow_object> {
   public:
      static const uint8_t space_id = PRIVATE_MESSAGE_SPACE_ID;
      static const uint8_t type_id  = follow_object_type;

      string        follower;
      string        following;
      set<string>   what; /// post, comments, votes, ignore
};

class feed_object : public abstract_object<feed_object> {
   public:
      static const uint8_t space_id = PRIVATE_MESSAGE_SPACE_ID;
      static const uint8_t type_id  = feed_object_type;
      account_id_type account;
      comment_id_type comment;
};


struct follow_operation {
    string          follower;
    string          following;
    set<string>     what; /// post, comments, votes
};

struct by_following_follower;
struct by_follower_following;
struct by_id;

using namespace boost::multi_index;

typedef multi_index_container<
   follow_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_following_follower >, 
            composite_key< follow_object,
               member< follow_object, string, &follow_object::following >,
               member< follow_object, string, &follow_object::follower >
            >,
            composite_key_compare< std::less<string>, std::less<string> >
      >,
      ordered_unique< tag< by_follower_following >, 
            composite_key< follow_object,
               member< follow_object, string, &follow_object::follower >,
               member< follow_object, string, &follow_object::following >
            >,
            composite_key_compare< std::less<string>, std::less<string> >
      >
   >
> follow_multi_index_type;

typedef graphene::db::generic_index< follow_object, follow_multi_index_type> follow_index;


struct by_account;
typedef multi_index_container<
    feed_object,
    indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_account >, 
            composite_key< feed_object,
               member< feed_object, account_id_type, &feed_object::account >,
               member< object, object_id_type, &object::id >
            >
      >
   >
> feed_multi_index_type;
     
typedef graphene::db::generic_index< feed_object, feed_multi_index_type> feed_index;



class follow_plugin : public steemit::app::plugin
{
   public:
      follow_plugin();

      std::string plugin_name()const override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override{};

      friend class detail::follow_plugin_impl;
      std::unique_ptr<detail::follow_plugin_impl> my;
};

class follow_api : public std::enable_shared_from_this<follow_api> {
   public:
      follow_api(){};
      follow_api(const app::api_context& ctx):_app(&ctx.app){} void on_api_startup(){} 

      vector<follow_object> get_followers( string to, string start, uint16_t limit )const;
      vector<follow_object> get_following( string from, string start, uint16_t limit )const;


   private:
      app::application* _app = nullptr;
};


} } //steemit::follow

FC_API( steemit::follow::follow_api, (get_followers)(get_following) );

FC_REFLECT_DERIVED( steemit::follow::follow_object, (graphene::db::object), (follower)(following)(what) );
FC_REFLECT( steemit::follow::follow_operation, (follower)(following)(what) )
FC_REFLECT_DERIVED( steemit::follow::feed_object, (graphene::db::object), (account)(comment) )
