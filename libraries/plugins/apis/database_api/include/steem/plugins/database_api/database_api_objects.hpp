#pragma once
#include <steem/chain/account_object.hpp>
#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/witness_objects.hpp>
#include <steem/chain/database.hpp>

namespace steem { namespace plugins { namespace database_api {

using namespace steem::chain;

typedef change_recovery_account_request_object api_change_recovery_account_request_object;
typedef block_summary_object                   api_block_summary_object;
typedef dynamic_global_property_object         api_dynamic_global_property_object;
typedef convert_request_object                 api_convert_request_object;
typedef escrow_object                          api_escrow_object;
typedef liquidity_reward_balance_object        api_liquidity_reward_balance_object;
typedef limit_order_object                     api_limit_order_object;
typedef withdraw_vesting_route_object          api_withdraw_vesting_route_object;
typedef decline_voting_rights_request_object   api_decline_voting_rights_request_object;
typedef witness_vote_object                    api_witness_vote_object;
typedef vesting_delegation_object              api_vesting_delegation_object;
typedef vesting_delegation_expiration_object   api_vesting_delegation_expiration_object;
typedef reward_fund_object                     api_reward_fund_object;

struct api_comment_object
{
   api_comment_object( const comment_object& o, const database& db ):
      id( o.id ),
      category( to_string( o.category ) ),
      parent_author( o.parent_author ),
      parent_permlink( to_string( o.parent_permlink ) ),
      author( o.author ),
      permlink( to_string( o.permlink ) ),
      last_update( o.last_update ),
      created( o.created ),
      active( o.active ),
      last_payout( o.last_payout ),
      depth( o.depth ),
      children( o.children ),
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

      const auto root = db.find( o.root_comment );

      if( root != nullptr )
      {
         root_author = root->author;
         root_permlink = to_string( root->permlink );
      }
#ifndef IS_LOW_MEM
      const auto& con = db.get< chain::comment_content_object, chain::by_comment >( o.id );
      title = to_string( con.title );
      body = to_string( con.body );
      json_metadata = to_string( con.json_metadata );
#endif
   }

   api_comment_object(){}

   comment_id_type   id;
   string            category;
   string            parent_author;
   string            parent_permlink;
   string            author;
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

   account_name_type root_author;
   string            root_permlink;

   asset             max_accepted_payout;
   uint16_t          percent_steem_dollars = 0;
   bool              allow_replies = false;
   bool              allow_votes = false;
   bool              allow_curation_rewards = false;
   vector< beneficiary_route_type > beneficiaries;
};

struct api_comment_vote_object
{
   api_comment_vote_object( const comment_vote_object& cv, const database& db ) :
      id( cv.id ),
      weight( cv.weight ),
      rshares( cv.rshares),
      vote_percent( cv.vote_percent ),
      last_update( cv.last_update ),
      num_changes( cv.num_changes )
   {
      voter = db.get( cv.voter ).name;
      auto comment = db.get( cv.comment );
      author = comment.author;
      permlink = to_string( comment.permlink );
   }

   comment_vote_id_type id;

   account_name_type    voter;
   account_name_type    author;
   string               permlink;
   uint64_t             weight = 0;
   int64_t              rshares = 0;
   int16_t              vote_percent = 0;
   time_point_sec       last_update;
   int8_t               num_changes = 0;
};

struct api_account_object
{
   api_account_object( const account_object& a, const database& db ) :
      id( a.id ),
      name( a.name ),
      memo_key( a.memo_key ),
      proxy( a.proxy ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      mined( a.mined ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      comment_count( a.comment_count ),
      lifetime_vote_count( a.lifetime_vote_count ),
      post_count( a.post_count ),
      can_vote( a.can_vote ),
      voting_manabar( a.voting_manabar ),
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
      last_post( a.last_post ),
      last_root_post( a.last_root_post ),
      last_vote_time( a.last_vote_time ),
      post_bandwidth( a.post_bandwidth ),
      pending_claimed_accounts( a.pending_claimed_accounts )
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
#ifndef IS_LOW_MEM
      const auto* maybe_meta = db.find< account_metadata_object, by_account >( id );
      if( maybe_meta )
         json_metadata = to_string( maybe_meta->json_metadata );
#endif

#ifdef STEEM_ENABLE_SMT
      const auto& by_control_account_index = db.get_index<smt_token_index>().indices().get<by_control_account>();
      auto smt_obj_itr = by_control_account_index.find( name );
      is_smt = smt_obj_itr != by_control_account_index.end();
#endif
   }


   api_account_object(){}

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
   account_name_type recovery_account;
   account_name_type reset_account;
   time_point_sec    last_account_recovery;
   uint32_t          comment_count = 0;
   uint32_t          lifetime_vote_count = 0;
   uint32_t          post_count = 0;

   bool              can_vote = false;
   util::manabar     voting_manabar;

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

   uint16_t          witnesses_voted_for = 0;

   time_point_sec    last_post;
   time_point_sec    last_root_post;
   time_point_sec    last_vote_time;
   uint32_t          post_bandwidth = 0;

   share_type        pending_claimed_accounts = 0;

   bool              is_smt = false;
};

struct api_owner_authority_history_object
{
   api_owner_authority_history_object( const owner_authority_history_object& o ) :
      id( o.id ),
      account( o.account ),
      previous_owner_authority( authority( o.previous_owner_authority ) ),
      last_valid_time( o.last_valid_time )
   {}

   api_owner_authority_history_object() {}

   owner_authority_history_id_type  id;

   account_name_type                account;
   authority                        previous_owner_authority;
   time_point_sec                   last_valid_time;
};

struct api_account_recovery_request_object
{
   api_account_recovery_request_object( const account_recovery_request_object& o ) :
      id( o.id ),
      account_to_recover( o.account_to_recover ),
      new_owner_authority( authority( o.new_owner_authority ) ),
      expires( o.expires )
   {}

   api_account_recovery_request_object() {}

   account_recovery_request_id_type id;
   account_name_type                account_to_recover;
   authority                        new_owner_authority;
   time_point_sec                   expires;
};

struct api_account_history_object
{

};

struct api_savings_withdraw_object
{
   api_savings_withdraw_object( const savings_withdraw_object& o ) :
      id( o.id ),
      from( o.from ),
      to( o.to ),
      memo( to_string( o.memo ) ),
      request_id( o.request_id ),
      amount( o.amount ),
      complete( o.complete )
   {}

   api_savings_withdraw_object() {}

   savings_withdraw_id_type   id;
   account_name_type          from;
   account_name_type          to;
   string                     memo;
   uint32_t                   request_id = 0;
   asset                      amount;
   time_point_sec             complete;
};

struct api_feed_history_object
{
   api_feed_history_object( const feed_history_object& f ) :
      id( f.id ),
      current_median_history( f.current_median_history ),
      price_history( f.price_history.begin(), f.price_history.end() )
   {}

   api_feed_history_object() {}

   feed_history_id_type id;
   price                current_median_history;
   deque< price >       price_history;
};

struct api_witness_object
{
   api_witness_object( const witness_object& w ) :
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
      hardfork_time_vote( w.hardfork_time_vote ),
      available_witness_account_subsidies( w.available_witness_account_subsidies )
   {}

   api_witness_object() {}

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
   int64_t           available_witness_account_subsidies = 0;
};

struct api_witness_schedule_object
{
   api_witness_schedule_object() {}

   api_witness_schedule_object( const witness_schedule_object& wso) :
      id( wso.id ),
      current_virtual_time( wso.current_virtual_time ),
      next_shuffle_block_num( wso.next_shuffle_block_num ),
      num_scheduled_witnesses( wso.num_scheduled_witnesses ),
      elected_weight( wso.elected_weight ),
      timeshare_weight( wso.timeshare_weight ),
      miner_weight( wso.miner_weight ),
      witness_pay_normalization_factor( wso.witness_pay_normalization_factor ),
      median_props( wso.median_props ),
      majority_version( wso.majority_version ),
      max_voted_witnesses( wso.max_voted_witnesses ),
      max_miner_witnesses( wso.max_miner_witnesses ),
      max_runner_witnesses( wso.max_runner_witnesses ),
      hardfork_required_witnesses( wso.hardfork_required_witnesses ),
      account_subsidy_rd( wso.account_subsidy_rd ),
      account_subsidy_witness_rd( wso.account_subsidy_witness_rd ),
      min_witness_account_subsidy_decay( wso.min_witness_account_subsidy_decay )
   {
      size_t n = wso.current_shuffled_witnesses.size();
      current_shuffled_witnesses.reserve( n );
      std::transform(wso.current_shuffled_witnesses.begin(), wso.current_shuffled_witnesses.end(),
                     std::back_inserter(current_shuffled_witnesses),
                     [](const account_name_type& s) -> std::string { return s; } );
                     // ^ fixed_string std::string operator used here.
   }

   witness_schedule_id_type   id;

   fc::uint128                current_virtual_time;
   uint32_t                   next_shuffle_block_num;
   vector<string>             current_shuffled_witnesses;   // fc::array<account_name_type,...> -> vector<string>
   uint8_t                    num_scheduled_witnesses;
   uint8_t                    elected_weight;
   uint8_t                    timeshare_weight;
   uint8_t                    miner_weight;
   uint32_t                   witness_pay_normalization_factor;
   chain_properties           median_props;
   version                    majority_version;

   uint8_t                    max_voted_witnesses;
   uint8_t                    max_miner_witnesses;
   uint8_t                    max_runner_witnesses;
   uint8_t                    hardfork_required_witnesses;

   rd_dynamics_params         account_subsidy_rd;
   rd_dynamics_params         account_subsidy_witness_rd;
   int64_t                    min_witness_account_subsidy_decay = 0;
};

struct api_signed_block_object : public signed_block
{
   api_signed_block_object( const signed_block& block ) : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const signed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }
   api_signed_block_object() {}

   block_id_type                 block_id;
   public_key_type               signing_key;
   vector< transaction_id_type > transaction_ids;
};

struct api_hardfork_property_object
{
   api_hardfork_property_object( const hardfork_property_object& h ) :
      id( h.id ),
      last_hardfork( h.last_hardfork ),
      current_hardfork_version( h.current_hardfork_version ),
      next_hardfork( h.next_hardfork ),
      next_hardfork_time( h.next_hardfork_time )
   {
      size_t n = h.processed_hardforks.size();
      processed_hardforks.reserve( n );

      for( size_t i = 0; i < n; i++ )
         processed_hardforks.push_back( h.processed_hardforks[i] );
   }

   api_hardfork_property_object() {}

   hardfork_property_id_type     id;
   vector< fc::time_point_sec >  processed_hardforks;
   uint32_t                      last_hardfork;
   protocol::hardfork_version    current_hardfork_version;
   protocol::hardfork_version    next_hardfork;
   fc::time_point_sec            next_hardfork_time;
};



struct order
{
   price                order_price;
   double               real_price; // dollars per steem
   share_type           steem;
   share_type           sbd;
   fc::time_point_sec   created;
};

struct order_book
{
   vector< order >      asks;
   vector< order >      bids;
};

} } } // steem::plugins::database_api

FC_REFLECT( steem::plugins::database_api::api_comment_object,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (title)(body)(json_metadata)(last_update)(created)(active)(last_payout)
             (depth)(children)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)(max_cashout_time)
             (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(author_rewards)(net_votes)
             (root_author)(root_permlink)
             (max_accepted_payout)(percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
          )

FC_REFLECT( steem::plugins::database_api::api_comment_vote_object,
             (id)(voter)(author)(permlink)(weight)(rshares)(vote_percent)(last_update)(num_changes)
          )

FC_REFLECT( steem::plugins::database_api::api_account_object,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_manabar)
             (balance)
             (savings_balance)
             (sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
             (savings_sbd_balance)(savings_sbd_seconds)(savings_sbd_seconds_last_update)(savings_sbd_last_interest_payment)(savings_withdraw_requests)
             (reward_sbd_balance)(reward_steem_balance)(reward_vesting_balance)(reward_vesting_steem)
             (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (curation_rewards)
             (posting_rewards)
             (proxied_vsf_votes)(witnesses_voted_for)
             (last_post)(last_root_post)(last_vote_time)
             (post_bandwidth)(pending_claimed_accounts)
             (is_smt)
          )

FC_REFLECT( steem::plugins::database_api::api_owner_authority_history_object,
             (id)
             (account)
             (previous_owner_authority)
             (last_valid_time)
          )

FC_REFLECT( steem::plugins::database_api::api_account_recovery_request_object,
             (id)
             (account_to_recover)
             (new_owner_authority)
             (expires)
          )

FC_REFLECT( steem::plugins::database_api::api_savings_withdraw_object,
             (id)
             (from)
             (to)
             (memo)
             (request_id)
             (amount)
             (complete)
          )

FC_REFLECT( steem::plugins::database_api::api_feed_history_object,
             (id)
             (current_median_history)
             (price_history)
          )

FC_REFLECT( steem::plugins::database_api::api_witness_object,
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
             (available_witness_account_subsidies)
          )

FC_REFLECT( steem::plugins::database_api::api_witness_schedule_object,
             (id)
             (current_virtual_time)
             (next_shuffle_block_num)
             (current_shuffled_witnesses)
             (num_scheduled_witnesses)
             (elected_weight)
             (timeshare_weight)
             (miner_weight)
             (witness_pay_normalization_factor)
             (median_props)
             (majority_version)
             (max_voted_witnesses)
             (max_miner_witnesses)
             (max_runner_witnesses)
             (hardfork_required_witnesses)
             (account_subsidy_rd)
             (account_subsidy_witness_rd)
             (min_witness_account_subsidy_decay)
          )

FC_REFLECT_DERIVED( steem::plugins::database_api::api_signed_block_object, (steem::protocol::signed_block),
                     (block_id)
                     (signing_key)
                     (transaction_ids)
                  )

FC_REFLECT( steem::plugins::database_api::api_hardfork_property_object,
            (id)
            (processed_hardforks)
            (last_hardfork)
            (current_hardfork_version)
            (next_hardfork)
            (next_hardfork_time)
          )

FC_REFLECT( steem::plugins::database_api::order, (order_price)(real_price)(steem)(sbd)(created) );

FC_REFLECT( steem::plugins::database_api::order_book, (asks)(bids) );
