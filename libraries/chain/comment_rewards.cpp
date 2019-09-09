#include <steem/chain/comment_rewards.hpp>

#include <steem/chain/comment_object.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/vesting.hpp>

#include <steem/chain/util/reward.hpp>

namespace steem { namespace chain {

struct comment_context
{
   comment_context( const comment_object& c, const rshare_context& r_ctx, const comment_votable_asset_options& va_opts, const t_vector< beneficiary_route_type >* b ) :
      id( c.id ),
      author( c.author ),
      permlink( to_string( c.permlink ) ),
      net_rshares( r_ctx.net_rshares ),
      abs_rshares( r_ctx.abs_rshares ),
      vote_rshares( r_ctx.vote_rshares ),
      total_vote_weight( r_ctx.total_vote_weight ),
      max_accepted_payout( va_opts.max_accepted_payout ),
      allow_curation_rewards( va_opts.allow_curation_rewards ),
      beneficiaries( b )
   {}

   comment_id_type      id;
   account_name_type    author;
   string               permlink;

   share_type           net_rshares;
   share_type           abs_rshares;
   share_type           vote_rshares;

   share_type           total_vote_weight;

   share_type           max_accepted_payout;
   bool                 allow_curation_rewards = true;

   const t_vector< beneficiary_route_type >* beneficiaries = nullptr;
};

template< typename CommentType >
share_type reward_comment( comment_context c_ctx, const CommentType& comment, const reward_fund_context& rf_ctx, const price& current_steem_price, database& db );

template< typename CommentType >
share_type reward_curators( share_type tokens, comment_context& c_ctx, const CommentType& comment, const asset_symbol_type symbol, database& db );

void process_comment_rewards( database& db )
{
   flat_map< asset_symbol_type, reward_fund_context > reward_funds;
   flat_map< asset_symbol_type, uint128_t > new_claims;
   const auto& steem_rf = db.get< reward_fund_object, by_id >( reward_fund_id_type() );

   reward_funds[ STEEM_SYMBOL ].recent_claims = steem_rf.recent_claims;
   reward_funds[ STEEM_SYMBOL ].reward_balance = steem_rf.reward_balance;
   reward_funds[ STEEM_SYMBOL ].last_update = steem_rf.last_update;
   reward_funds[ STEEM_SYMBOL ].author_reward_curve = steem_rf.author_reward_curve;
   reward_funds[ STEEM_SYMBOL ].curation_reward_curve = steem_rf.curation_reward_curve;
   reward_funds[ STEEM_SYMBOL ].content_constant = steem_rf.content_constant;
   reward_funds[ STEEM_SYMBOL ].percent_curation_rewards = steem_rf.percent_curation_rewards;
   new_claims[ STEEM_SYMBOL ] = 0;

   const auto& cidx = db.get_index< comment_index >().indices().get< by_cashout_time >();
   auto current = cidx.begin();

   fc::time_point_sec now = db.head_block_time();

   while( current != cidx.end() && current->cashout_time <= now )
   {
      if( current->net_rshares > 0 )
      {
         new_claims[ STEEM_SYMBOL ] += util::evaluate_reward_curve( current->net_rshares.value, steem_rf.author_reward_curve, steem_rf.content_constant );
      }

      for( auto& smt_rshare : current->smt_rshares )
      {
         auto smt_rf = reward_funds.find( smt_rshare.first );
         if( smt_rf == reward_funds.end() )
         {
            const auto& smt_fund = db.get< smt_token_object, by_symbol >( smt_rshare.first );

            reward_fund_context rf_ctx;
            rf_ctx.recent_claims = smt_fund.recent_claims;
            rf_ctx.reward_balance = smt_fund.reward_balance;
            rf_ctx.last_update = smt_fund.last_reward_update;
            rf_ctx.author_reward_curve = smt_fund.author_reward_curve;
            rf_ctx.curation_reward_curve = smt_fund.curation_reward_curve;
            rf_ctx.content_constant = smt_fund.content_constant;
            rf_ctx.percent_curation_rewards = smt_fund.percent_curation_rewards;
            reward_funds.insert_or_assign( smt_rshare.first, std::move( rf_ctx ) );

            new_claims[ smt_rshare.first ] = 0;
         }

         if( smt_rshare.second.net_rshares > 0 )
         {
            new_claims[ smt_rshare.first ] += util::evaluate_reward_curve( smt_rshare.second.net_rshares.value, smt_rf->second.author_reward_curve, smt_rf->second.content_constant );
         }
      }

      ++current;
   }

   for( auto& rf_ctx : reward_funds )
   {
      fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME_HF19;
      rf_ctx.second.recent_claims -= ( rf_ctx.second.recent_claims * ( now - rf_ctx.second.last_update ).to_seconds() ) / decay_time.to_seconds();
      rf_ctx.second.recent_claims += new_claims[ rf_ctx.first ];
      rf_ctx.second.last_update = now;
   }

   const auto current_steem_price = db.get_feed_history().current_median_history;
   current = cidx.begin();
   util::comment_reward_context ctx;

   while( current != cidx.end() && current->cashout_time <= now )
   {
      ctx.total_claims = reward_funds[ STEEM_SYMBOL ].recent_claims;
      ctx.reward_fund = reward_funds[ STEEM_SYMBOL ].reward_balance.amount;
      reward_funds[ STEEM_SYMBOL ].tokens_awarded += db.cashout_comment_helper( ctx, *current, current_steem_price, false );
      /*reward_funds[ STEEM_SYMBOL ].tokens_awarded += reward_comment(
         current->author,
         *current,
         current->max_accepted_payout.amount,
         reward_funds[ STEEM_SYMBOL ],
         current_steem_price,
         &(current->beneficiaries)
         db
      );
      */

      for( auto smt_rshare : current->smt_rshares )
      {
         auto beneficiaries = db.find< comment_smt_beneficiaries_object, by_comment_symbol >( boost::make_tuple( current->id, smt_rshare.first ) );
         auto va_opts = current->allowed_vote_assets.find( smt_rshare.first );
         comment_context c_ctx( *current, smt_rshare.second, va_opts->second, beneficiaries != nullptr ? &(beneficiaries->beneficiaries) : nullptr );

         // The find here is safe because the comment has rshares for that symbol already, which requires the vote assets to exits
         reward_funds[ smt_rshare.first ].tokens_awarded += reward_comment(
            c_ctx,
            smt_rshare.second,
            reward_funds[ smt_rshare.first ],
            current_steem_price,
            db
         );

         db.modify( *current, [&]( comment_object& c )
         {
            c.net_rshares = c_ctx.net_rshares;
            c.abs_rshares = c_ctx.abs_rshares;
            c.vote_rshares = c_ctx.vote_rshares;
            c.total_vote_weight = c_ctx.total_vote_weight.value;
#ifndef IS_LOW_MEM
            //c.author_rewards += c_ctx.author_tokens;
#endif
         });
      }

      current = cidx.begin();
   }

   for( auto& rf_ctx : reward_funds )
   {
      if( rf_ctx.first == STEEM_SYMBOL )
      {
         db.modify( steem_rf, [&]( reward_fund_object& r )
         {
            r.recent_claims = rf_ctx.second.recent_claims;
            r.reward_balance -= asset( rf_ctx.second.tokens_awarded, STEEM_SYMBOL );
            r.last_update = rf_ctx.second.last_update;
         });
      }
      else
      {
         db.modify( db.get< smt_token_object, by_symbol >( rf_ctx.first ), [&]( smt_token_object& t )
         {
            t.recent_claims = rf_ctx.second.recent_claims;
            t.reward_balance -= asset( rf_ctx.second.tokens_awarded, rf_ctx.second.reward_balance.symbol );
            t.last_reward_update = rf_ctx.second.last_update;
         });
      }
   }
}

template< typename CommentType >
share_type reward_comment( comment_context c_ctx, const CommentType& comment, const reward_fund_context& rf_ctx, const price& current_steem_price, database& db )
{
   try
   {
      share_type claimed_reward = 0;
      share_type author_tokens = 0;

      if( comment.net_rshares > 0 )
      {
         util::comment_reward_context ctx;
         ctx.rshares = comment.net_rshares;
         ctx.reward_curve = rf_ctx.author_reward_curve;
         ctx.content_constant = rf_ctx.content_constant;
         ctx.total_claims = rf_ctx.recent_claims;
         ctx.reward_fund = rf_ctx.reward_balance.amount.value;
         ctx.reward_weight = STEEM_100_PERCENT; // Not used past HF 17, set to 100%

         uint64_t reward = util::get_rshare_reward( ctx );
         reward = std::min( reward, (uint64_t)(c_ctx.max_accepted_payout.value) );

         uint128_t reward_tokens = uint128_t( reward );

         if( reward_tokens > 0 )
         {
            share_type curation_tokens = ( ( reward_tokens * rf_ctx.percent_curation_rewards ) / STEEM_100_PERCENT ).to_uint64();
            author_tokens = reward_tokens.to_uint64() - curation_tokens;

            curation_tokens -= reward_curators( curation_tokens, c_ctx, comment, rf_ctx.reward_balance.symbol, db );

            share_type total_beneficiary = 0;
            claimed_reward = author_tokens + curation_tokens;

            // I do not like having to do this as a nullptr check, but the object might not exist for SMT beneficiaries
            if( c_ctx.beneficiaries != nullptr )
            {
               for( auto& b : *(c_ctx.beneficiaries) )
               {
                  auto benefactor_tokens = ( author_tokens * b.weight ) / STEEM_100_PERCENT;
                  auto benefactor_vesting_tokens = benefactor_tokens;
                  auto vop = comment_benefactor_reward_operation( b.account, c_ctx.author, c_ctx.permlink, asset( 0, SBD_SYMBOL ), asset( 0, STEEM_SYMBOL ), asset( 0, rf_ctx.reward_balance.symbol.get_paired_symbol() ) );

                  create_vesting2( db, db.get_account( b.account ), asset( benefactor_vesting_tokens.value, rf_ctx.reward_balance.symbol ), true,
                  [&]( const asset& reward )
                  {
                     vop.vesting_payout = reward;
                     db.pre_push_virtual_operation( vop );
                  });

                  db.post_push_virtual_operation( vop );
                  total_beneficiary += benefactor_tokens;
               }
            }

            author_tokens -= total_beneficiary;
            auto vesting_token = author_tokens;

            const auto& author = db.get_account( c_ctx.author );
            // TODO Update author reward operation to hold all tokens at once
            operation vop = author_reward_operation( c_ctx.author, c_ctx.permlink, asset( 0, SBD_SYMBOL ), asset( 0, STEEM_SYMBOL ), asset( 0, VESTS_SYMBOL ) );

            create_vesting2( db, author, asset( vesting_token.value, rf_ctx.reward_balance.symbol ), true,
            [&]( const asset& vesting_payout )
            {
               vop.get< author_reward_operation >().vesting_payout = vesting_payout;
               db.pre_push_virtual_operation( vop );
            } );

            FC_TODO( "Adjust total payout for SMTs" );

            db.post_push_virtual_operation( vop );
            vop = comment_reward_operation( c_ctx.author, c_ctx.permlink, asset( claimed_reward, rf_ctx.reward_balance.symbol ) );
            db.pre_push_virtual_operation( vop );
            db.post_push_virtual_operation( vop );

            FC_TODO( "Update author and posting rewards for SMTs" );
         }
      }

      /**
      * A payout is only made for positive rshares, negative rshares hang around
      * for the next time this post might get an upvote.
      */
      if( c_ctx.net_rshares > 0 )
         c_ctx.net_rshares = 0;
      c_ctx.abs_rshares  = 0;
      c_ctx.vote_rshares = 0;
      c_ctx.total_vote_weight = 0;
#ifndef IS_LOW_MEM
      //c_ctx.author_rewards += author_tokens;
#endif

      return claimed_reward;
   } FC_CAPTURE_AND_RETHROW( (comment)(rf_ctx)(current_steem_price) );
}

template< typename CommentType >
share_type reward_curators( share_type tokens, comment_context& c_ctx, const CommentType& comment, const asset_symbol_type symbol, database& db )
{
   struct cmp
   {
      bool operator()( const comment_vote_object* obj, const comment_vote_object* obj2 ) const
      {
         if( obj->weight == obj2->weight )
            return obj->voter < obj2->voter;
         else
            return obj->weight > obj2->weight;
      }
   };

   try {
      uint128_t total_weight( c_ctx.total_vote_weight.value );
      share_type unclaimed_rewards = tokens;

      if( !c_ctx.allow_curation_rewards )
      {
         unclaimed_rewards = 0;
         tokens = 0;
      }
      else if( c_ctx.total_vote_weight > 0 )
      {
         const auto& cvidx = db.get_index< comment_vote_index, by_comment_symbol_voter >();
         auto itr = cvidx.lower_bound( boost::make_tuple( c_ctx.id, symbol ) );

         std::set< const comment_vote_object*, cmp > proxy_set;
         while( itr != cvidx.end() && itr->comment == c_ctx.id && itr->symbol == symbol )
         {
            proxy_set.insert( &(*itr) );
            ++itr;
         }

         for( auto& item : proxy_set )
         {
            try
            {
               uint128_t weight( item->weight );
               auto claim = ( (tokens.value * weight ) / total_weight ).to_uint64();

               if( claim > 0 )
               {
                  unclaimed_rewards -= claim;
                  const auto& voter = db.get( item->voter );

                  operation vop = curation_reward_operation( voter.name, asset( 0, symbol.get_paired_symbol() ), c_ctx.author, c_ctx.permlink );

                  create_vesting2( db, voter, asset( claim, symbol ), true,
                  [&]( const asset& reward )
                  {
                     vop.get< curation_reward_operation >().reward = reward;
                     db.pre_push_virtual_operation( vop );
                  });

                  // TODO Curtation reward count for SMTs

                  db.post_push_virtual_operation( vop );
               }
            } FC_CAPTURE_AND_RETHROW( (*item) )
         }
      }

      tokens -= unclaimed_rewards;
      return unclaimed_rewards;

   } FC_CAPTURE_AND_RETHROW( (tokens)(comment)(symbol) )
}

} } // steem::chain

/*
FC_REFLECT( steem::chain::comment_context,
            (id)
            (author)
            (permlink)
            (net_rshares)
            (abs_rshares)
            (vote_rshares)
            (total_vote_weight)
            (max_accepted_payout)
            (allow_curation_rewards) )
*/
