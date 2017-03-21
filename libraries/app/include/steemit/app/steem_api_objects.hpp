#pragma once
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/block_summary_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/global_property_object.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/transaction_object.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <steemit/tags/tags_plugin.hpp>

namespace steemit { namespace app {

using namespace steemit::chain;

/*struct limit_order
{
   limit_order( chain::limit_order_object& o ):
      id( o.id ),
      created( o.created ),
      expiration( o.expiration ),
      seller( o.seller ),
      orderid( o.orderid ),
      for_sale( o.for_sale ),
      sell_price( o.sell_price )
   {}

   limit_order(){}

   chain::limit_order_id_type id;
   time_point_sec             created;
   time_point_sec             expiration;
   account_name_type          seller;
   uint32_t                   orderid = 0;
   share_type                 for_sale;
   price                      sell_price;
};*/

typedef chain::change_recovery_account_request_object  change_recovery_account_request_api_obj;
typedef chain::block_summary_object                    block_summary_api_obj;
typedef chain::comment_vote_object                     comment_vote_api_obj;
typedef chain::dynamic_global_property_object          dynamic_global_property_api_obj;
typedef chain::convert_request_object                  convert_request_api_obj;
typedef chain::escrow_object                           escrow_api_obj;
typedef chain::liquidity_reward_balance_object         liquidity_reward_balance_api_obj;
typedef chain::limit_order_object                      limit_order_api_obj;
typedef chain::withdraw_vesting_route_object           withdraw_vesting_route_api_obj;
typedef chain::decline_voting_rights_request_object    decline_voting_rights_request_api_obj;
typedef chain::witness_vote_object                     witness_vote_api_obj;
typedef chain::witness_schedule_object                 witness_schedule_api_obj;
typedef chain::account_bandwidth_object                account_bandwidth_api_obj;
typedef chain::vesting_delegation_object               vesting_delegation_api_obj;
typedef chain::vesting_delegation_expiration_object    vesting_delegation_expiration_api_obj;
typedef chain::reward_fund_object                      reward_fund_api_obj;

struct comment_api_obj
{
   comment_api_obj( const chain::comment_object& o ):
      id( o.id ),
      category( to_string( o.category ) ),
      parent_author( o.parent_author ),
      parent_permlink( to_string( o.parent_permlink ) ),
      author( o.author ),
      permlink( to_string( o.permlink ) ),
      title( to_string( o.title ) ),
      body( to_string( o.body ) ),
      json_metadata( to_string( o.json_metadata ) ),
      last_update( o.last_update ),
      created( o.created ),
      active( o.active ),
      last_payout( o.last_payout ),
      depth( o.depth ),
      children( o.children ),
      children_rshares2( o.children_rshares2 ),
      net_rshares( o.net_rshares ),
      abs_rshares( o.abs_rshares ),
      vote_rshares( o.vote_rshares ),
      children_abs_rshares( o.children_abs_rshares ),
      cashout_time( o.cashout_time ),
      max_cashout_time( o.max_cashout_time ),
      total_vote_weight( o.total_vote_weight ),
      reward_weight( o.reward_weight ),
      total_payout_value( o.total_payout_value ),
      curator_payout_value( o.curator_payout_value ),
      author_rewards( o.author_rewards ),
      net_votes( o.net_votes ),
      root_comment( o.root_comment ),
      max_accepted_payout( o.max_accepted_payout ),
      percent_steem_dollars( o.percent_steem_dollars ),
      allow_replies( o.allow_replies ),
      allow_votes( o.allow_votes ),
      allow_curation_rewards( o.allow_curation_rewards )
   {
      for( auto& route : o.beneficiaries )
      {
         beneficiaries.push_back( route );
      }
   }

   comment_api_obj(){}

   comment_id_type   id;
   string            category;
   account_name_type parent_author;
   string            parent_permlink;
   account_name_type author;
   string            permlink;

   string            title;
   string            body;
   string            json_metadata;
   time_point_sec    last_update;
   time_point_sec    created;
   time_point_sec    active;
   time_point_sec    last_payout;

   uint8_t           depth = 0;
   uint32_t          children = 0;

   uint128_t         children_rshares2;

   share_type        net_rshares;
   share_type        abs_rshares;
   share_type        vote_rshares;

   share_type        children_abs_rshares;
   time_point_sec    cashout_time;
   time_point_sec    max_cashout_time;
   uint64_t          total_vote_weight = 0;

   uint16_t          reward_weight = 0;

   asset             total_payout_value;
   asset             curator_payout_value;

   share_type        author_rewards;

   int32_t           net_votes = 0;

   comment_id_type   root_comment;

   asset             max_accepted_payout;
   uint16_t          percent_steem_dollars = 0;
   bool              allow_replies = false;
   bool              allow_votes = false;
   bool              allow_curation_rewards = false;
   vector< beneficiary_route_type > beneficiaries;
};

struct category_api_obj
{
   category_api_obj( const chain::category_object& c ) :
      id( c.id ),
      name( to_string( c.name ) ),
      abs_rshares( c.abs_rshares ),
      total_payouts( c.total_payouts ),
      discussions( c.discussions ),
      last_update( c.last_update )
   {}

   category_api_obj() {}

   category_id_type     id;
   string               name;
   share_type           abs_rshares;
   asset                total_payouts;
   uint32_t             discussions = 0;
   time_point_sec       last_update;
};

struct tag_api_obj
{
   tag_api_obj( const tags::tag_stats_object& o ) :
      name( o.tag ),
      total_children_rshares2(o.total_children_rshares2),
      total_payouts(o.total_payout),
      net_votes(o.net_votes),
      top_posts(o.top_posts),
      comments(o.comments) {}

   tag_api_obj() {}

   string               name;
   fc::uint128_t        total_children_rshares2;
   asset                total_payouts;
   int32_t              net_votes = 0;
   uint32_t             top_posts = 0;
   uint32_t             comments = 0;
};

struct account_api_obj
{
   account_api_obj( const chain::account_object& a, const chain::database& db ) :
      id( a.id ),
      name( a.name ),
      memo_key( a.memo_key ),
      json_metadata( to_string( a.json_metadata ) ),
      proxy( a.proxy ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      mined( a.mined ),
      owner_challenged( a.owner_challenged ),
      active_challenged( a.active_challenged ),
      last_owner_proved( a.last_owner_proved ),
      last_active_proved( a.last_active_proved ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      comment_count( a.comment_count ),
      lifetime_vote_count( a.lifetime_vote_count ),
      post_count( a.post_count ),
      can_vote( a.can_vote ),
      voting_power( a.voting_power ),
      last_vote_time( a.last_vote_time ),
      balance( a.balance ),
      savings_balance( a.savings_balance ),
      sbd_balance( a.sbd_balance ),
      sbd_seconds( a.sbd_seconds ),
      sbd_seconds_last_update( a.sbd_seconds_last_update ),
      sbd_last_interest_payment( a.sbd_last_interest_payment ),
      savings_sbd_balance( a.savings_sbd_balance ),
      savings_sbd_seconds( a.savings_sbd_seconds ),
      savings_sbd_seconds_last_update( a.savings_sbd_seconds_last_update ),
      savings_sbd_last_interest_payment( a.savings_sbd_last_interest_payment ),
      savings_withdraw_requests( a.savings_withdraw_requests ),
      reward_sbd_balance( a.reward_sbd_balance ),
      reward_steem_balance( a.reward_steem_balance ),
      reward_vesting_balance( a.reward_vesting_balance ),
      reward_vesting_steem( a.reward_vesting_steem ),
      curation_rewards( a.curation_rewards ),
      posting_rewards( a.posting_rewards ),
      vesting_shares( a.vesting_shares ),
      delegated_vesting_shares( a.delegated_vesting_shares ),
      received_vesting_shares( a.received_vesting_shares ),
      vesting_withdraw_rate( a.vesting_withdraw_rate ),
      next_vesting_withdrawal( a.next_vesting_withdrawal ),
      withdrawn( a.withdrawn ),
      to_withdraw( a.to_withdraw ),
      withdraw_routes( a.withdraw_routes ),
      witnesses_voted_for( a.witnesses_voted_for ),
      last_post( a.last_post )
   {
      size_t n = a.proxied_vsf_votes.size();
      proxied_vsf_votes.reserve( n );
      for( size_t i=0; i<n; i++ )
         proxied_vsf_votes.push_back( a.proxied_vsf_votes[i] );

      const auto& auth = db.get< account_authority_object, by_account >( name );
      owner = authority( auth.owner );
      active = authority( auth.active );
      posting = authority( auth.posting );
      last_owner_update = auth.last_owner_update;

      auto post = db.find< account_bandwidth_object, by_account_bandwidth_type >( boost::make_tuple( name, bandwidth_type::post ) );
      if( post != nullptr )
      {
         last_root_post = post->last_bandwidth_update;
         post_bandwidth = post->average_bandwidth;
      }

      auto forum = db.find< account_bandwidth_object, by_account_bandwidth_type >( boost::make_tuple( name, bandwidth_type::forum ) );
      if( forum != nullptr )
      {
         new_average_bandwidth = forum->average_bandwidth;
      }

      auto market = db.find< account_bandwidth_object, by_account_bandwidth_type >( boost::make_tuple( name, bandwidth_type::market ) );
      if( market != nullptr )
      {
         new_average_market_bandwidth = market->average_bandwidth;
      }
   }


   account_api_obj(){}

   account_id_type   id;

   account_name_type name;
   authority         owner;
   authority         active;
   authority         posting;
   public_key_type   memo_key;
   string            json_metadata;
   account_name_type proxy;

   time_point_sec    last_owner_update;
   time_point_sec    last_account_update;

   time_point_sec    created;
   bool              mined = false;
   bool              owner_challenged = false;
   bool              active_challenged = false;
   time_point_sec    last_owner_proved;
   time_point_sec    last_active_proved;
   account_name_type recovery_account;
   account_name_type reset_account;
   time_point_sec    last_account_recovery;
   uint32_t          comment_count = 0;
   uint32_t          lifetime_vote_count = 0;
   uint32_t          post_count = 0;

   bool              can_vote = false;
   uint16_t          voting_power = 0;
   time_point_sec    last_vote_time;

   asset             balance;
   asset             savings_balance;

   asset             sbd_balance;
   uint128_t         sbd_seconds;
   time_point_sec    sbd_seconds_last_update;
   time_point_sec    sbd_last_interest_payment;

   asset             savings_sbd_balance;
   uint128_t         savings_sbd_seconds;
   time_point_sec    savings_sbd_seconds_last_update;
   time_point_sec    savings_sbd_last_interest_payment;

   uint8_t           savings_withdraw_requests = 0;

   asset             reward_sbd_balance;
   asset             reward_steem_balance;
   asset             reward_vesting_balance;
   asset             reward_vesting_steem;

   share_type        curation_rewards;
   share_type        posting_rewards;

   asset             vesting_shares;
   asset             delegated_vesting_shares;
   asset             received_vesting_shares;
   asset             vesting_withdraw_rate;
   time_point_sec    next_vesting_withdrawal;
   share_type        withdrawn;
   share_type        to_withdraw;
   uint16_t          withdraw_routes = 0;

   vector< share_type > proxied_vsf_votes;

   uint16_t          witnesses_voted_for;

   share_type        average_bandwidth = 0;
   share_type        lifetime_bandwidth = 0;
   time_point_sec    last_bandwidth_update;

   share_type        average_market_bandwidth = 0;
   time_point_sec    last_market_bandwidth_update;
   time_point_sec    last_post;
   time_point_sec    last_root_post;
   share_type        post_bandwidth = STEEMIT_100_PERCENT;

   share_type        new_average_bandwidth;
   share_type        new_average_market_bandwidth;
};

struct owner_authority_history_api_obj
{
   owner_authority_history_api_obj( const chain::owner_authority_history_object& o ) :
      id( o.id ),
      account( o.account ),
      previous_owner_authority( authority( o.previous_owner_authority ) ),
      last_valid_time( o.last_valid_time )
   {}

   owner_authority_history_api_obj() {}

   owner_authority_history_id_type  id;

   account_name_type                account;
   authority                        previous_owner_authority;
   time_point_sec                   last_valid_time;
};

struct account_recovery_request_api_obj
{
   account_recovery_request_api_obj( const chain::account_recovery_request_object& o ) :
      id( o.id ),
      account_to_recover( o.account_to_recover ),
      new_owner_authority( authority( o.new_owner_authority ) ),
      expires( o.expires )
   {}

   account_recovery_request_api_obj() {}

   account_recovery_request_id_type id;
   account_name_type                account_to_recover;
   authority                        new_owner_authority;
   time_point_sec                   expires;
};

struct account_history_api_obj
{

};

struct savings_withdraw_api_obj
{
   savings_withdraw_api_obj( const chain::savings_withdraw_object& o ) :
      id( o.id ),
      from( o.from ),
      to( o.to ),
      memo( to_string( o.memo ) ),
      request_id( o.request_id ),
      amount( o.amount ),
      complete( o.complete )
   {}

   savings_withdraw_api_obj() {}

   savings_withdraw_id_type   id;
   account_name_type          from;
   account_name_type          to;
   string                     memo;
   uint32_t                   request_id = 0;
   asset                      amount;
   time_point_sec             complete;
};

struct feed_history_api_obj
{
   feed_history_api_obj( const chain::feed_history_object& f ) :
      id( f.id ),
      current_median_history( f.current_median_history ),
      price_history( f.price_history.begin(), f.price_history.end() )
   {}

   feed_history_api_obj() {}

   feed_history_id_type id;
   price                current_median_history;
   deque< price >       price_history;
};

struct witness_api_obj
{
   witness_api_obj( const chain::witness_object& w ) :
      id( w.id ),
      owner( w.owner ),
      created( w.created ),
      url( to_string( w.url ) ),
      total_missed( w.total_missed ),
      last_aslot( w.last_aslot ),
      last_confirmed_block_num( w.last_confirmed_block_num ),
      pow_worker( w.pow_worker ),
      signing_key( w.signing_key ),
      props( w.props ),
      sbd_exchange_rate( w.sbd_exchange_rate ),
      last_sbd_exchange_update( w.last_sbd_exchange_update ),
      votes( w.votes ),
      virtual_last_update( w.virtual_last_update ),
      virtual_position( w.virtual_position ),
      virtual_scheduled_time( w.virtual_scheduled_time ),
      last_work( w.last_work ),
      running_version( w.running_version ),
      hardfork_version_vote( w.hardfork_version_vote ),
      hardfork_time_vote( w.hardfork_time_vote )
   {}

   witness_api_obj() {}

   witness_id_type   id;
   account_name_type owner;
   time_point_sec    created;
   string            url;
   uint32_t          total_missed = 0;
   uint64_t          last_aslot = 0;
   uint64_t          last_confirmed_block_num = 0;
   uint64_t          pow_worker = 0;
   public_key_type   signing_key;
   chain_properties  props;
   price             sbd_exchange_rate;
   time_point_sec    last_sbd_exchange_update;
   share_type        votes;
   fc::uint128       virtual_last_update;
   fc::uint128       virtual_position;
   fc::uint128       virtual_scheduled_time;
   digest_type       last_work;
   version           running_version;
   hardfork_version  hardfork_version_vote;
   time_point_sec    hardfork_time_vote;
};

} } // steemit::app

FC_REFLECT( steemit::app::comment_api_obj,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (title)(body)(json_metadata)(last_update)(created)(active)(last_payout)
             (depth)(children)(children_rshares2)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)(max_cashout_time)
             (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(author_rewards)(net_votes)(root_comment)
             (max_accepted_payout)(percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
          )

FC_REFLECT( steemit::app::category_api_obj,
             (id)(name)(abs_rshares)(total_payouts)(discussions)(last_update)
          )

FC_REFLECT( steemit::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(mined)
             (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
             (balance)
             (savings_balance)
             (sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
             (savings_sbd_balance)(savings_sbd_seconds)(savings_sbd_seconds_last_update)(savings_sbd_last_interest_payment)(savings_withdraw_requests)
             (reward_sbd_balance)(reward_steem_balance)(reward_vesting_balance)(reward_vesting_steem)
             (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (curation_rewards)
             (posting_rewards)
             (proxied_vsf_votes)(witnesses_voted_for)
             (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
             (average_market_bandwidth)(last_market_bandwidth_update)
             (last_post)(last_root_post)(post_bandwidth)
             (new_average_bandwidth)(new_average_market_bandwidth)
          )

FC_REFLECT( steemit::app::owner_authority_history_api_obj,
             (id)
             (account)
             (previous_owner_authority)
             (last_valid_time)
          )

FC_REFLECT( steemit::app::account_recovery_request_api_obj,
             (id)
             (account_to_recover)
             (new_owner_authority)
             (expires)
          )

FC_REFLECT( steemit::app::savings_withdraw_api_obj,
             (id)
             (from)
             (to)
             (memo)
             (request_id)
             (amount)
             (complete)
          )

FC_REFLECT( steemit::app::feed_history_api_obj,
             (id)
             (current_median_history)
             (price_history)
          )

FC_REFLECT( steemit::app::tag_api_obj,
            (name)
            (total_children_rshares2)
            (total_payouts)
            (net_votes)
            (top_posts)
            (comments)
          )

FC_REFLECT( steemit::app::witness_api_obj,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)
             (props)
             (sbd_exchange_rate)(last_sbd_exchange_update)
             (last_work)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )
