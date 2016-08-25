#include <steemit/chain/database.hpp>
#include <steemit/chain/generic_json_evaluator_registry.hpp>
#include <steemit/chain/steem_evaluator.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/witness_objects.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace steemit { namespace chain {
   using fc::uint128_t;

inline void validate_permlink_0_1( const string& permlink )
{
   FC_ASSERT( permlink.size() > STEEMIT_MIN_PERMLINK_LENGTH && permlink.size() < STEEMIT_MAX_PERMLINK_LENGTH, "permlink is not a valid size" );

   for( auto c : permlink )
   {
      switch( c )
      {
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
         case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
         case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '0':
         case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
         case '-':
            break;
         default:
            FC_ASSERT( false, "Invalid permlink character: ${s}", ("s", std::string() + c ) );
      }
   }
}



void witness_update_evaluator::do_apply( const witness_update_operation& o )
{
   db().get_account( o.owner ); // verify owner exists

   if ( db().has_hardfork( STEEMIT_HARDFORK_0_1 ) ) FC_ASSERT( o.url.size() <= STEEMIT_MAX_WITNESS_URL_LENGTH, "url is too long" );

   const auto& by_witness_name_idx = db().get_index_type< witness_index >().indices().get< by_name >();
   auto wit_itr = by_witness_name_idx.find( o.owner );
   if( wit_itr != by_witness_name_idx.end() )
   {
      db().modify( *wit_itr, [&]( witness_object& w ) {
         w.url                = o.url;
         w.signing_key        = o.block_signing_key;
         w.props              = o.props;
      });
   }
   else
   {
      db().create< witness_object >( [&]( witness_object& w ) {
         w.owner              = o.owner;
         w.url                = o.url;
         w.signing_key        = o.block_signing_key;
         w.created            = db().head_block_time();
         w.props              = o.props;
      });
   }
}

void account_create_evaluator::do_apply( const account_create_operation& o )
{
   const auto& creator = db().get_account( o.creator );

   const auto& props = db().get_dynamic_global_properties();

   FC_ASSERT( creator.balance >= o.fee, "Insufficient balance to create account", ( "creator.balance", creator.balance )( "required", o.fee ) );

   if( db().has_hardfork( STEEMIT_HARDFORK_0_1 ) )  {
      const witness_schedule_object& wso = db().get_witness_schedule_object();
      FC_ASSERT( o.fee >= wso.median_props.account_creation_fee, "Insufficient Fee: ${f} required, ${p} provided",
                 ("f", wso.median_props.account_creation_fee)
                 ("p", o.fee) );
   }

   db().modify( creator, [&]( account_object& c ){
      c.balance -= o.fee;
   });

   const auto& new_account = db().create< account_object >( [&]( account_object& acc )
   {
      acc.name = o.new_account_name;
      acc.owner = o.owner;
      acc.active = o.active;
      acc.posting = o.posting;
      acc.memo_key = o.memo_key;
      acc.last_owner_update = fc::time_point_sec::min();
      acc.created = props.time;
      acc.last_vote_time = props.time;
      acc.mined = false;

      if( !db().has_hardfork( STEEMIT_HARDFORK_0_11__169 ) )
         acc.recovery_account = "steem";
      else
         acc.recovery_account = o.creator;


      #ifndef IS_LOW_MEM
         acc.json_metadata = o.json_metadata;
      #endif
   });

   if( o.fee.amount > 0 )
      db().create_vesting( new_account, o.fee );
}


void account_update_evaluator::do_apply( const account_update_operation& o )
{
   if( db().has_hardfork( STEEMIT_HARDFORK_0_1 ) ) FC_ASSERT( o.account != STEEMIT_TEMP_ACCOUNT, "cannot update temp account" );

   const auto& account = db().get_account( o.account );

   if( o.owner )
   {
#ifndef IS_TESTNET
      if( db().has_hardfork( STEEMIT_HARDFORK_0_11 ) )
         FC_ASSERT( db().head_block_time() - account.last_owner_update > STEEMIT_OWNER_UPDATE_LIMIT, "can only update owner authority once a minute" );
#endif

      db().update_owner_authority( account, *o.owner );
   }

   db().modify( account, [&]( account_object& acc )
   {
      if( o.active ) acc.active = *o.active;
      if( o.posting ) acc.posting = *o.posting;

      if( o.memo_key != public_key_type() )
            acc.memo_key = o.memo_key;

      if( ( o.active || o.owner ) && acc.active_challenged )
      {
         acc.active_challenged = false;
         acc.last_active_proved = db().head_block_time();
      }

      acc.last_account_update = db().head_block_time();

      #ifndef IS_LOW_MEM
        if ( o.json_metadata.size() > 0 )
            acc.json_metadata = o.json_metadata;
      #endif
   });

}


/**
 *  Because net_rshares is 0 there is no need to update any pending payout calculations or parent posts.
 */
void delete_comment_evaluator::do_apply( const delete_comment_operation& o ) {
   if( db().has_hardfork( STEEMIT_HARDFORK_0_10 ) )
   {
      const auto& auth = db().get_account( o.author );
      FC_ASSERT( !(auth.owner_challenged || auth.active_challenged ), "cannot process operation because account is currently challenged" );
   }

   const auto& comment = db().get_comment( o.author, o.permlink );
   FC_ASSERT( comment.children == 0, "comment cannot have any replies" );

   if( db().is_producing() ) {
      FC_ASSERT( comment.net_rshares <= 0, "comment cannot have any net positive votes" );
   }
   if( comment.net_rshares > 0 ) return;

   const auto& vote_idx = db().get_index_type<comment_vote_index>().indices().get<by_comment_voter>();

   auto vote_itr = vote_idx.lower_bound( comment_id_type(comment.id) );
   while( vote_itr != vote_idx.end() && vote_itr->comment == comment.id ) {
      const auto& cur_vote = *vote_itr;
      ++vote_itr;
      db().remove(cur_vote);
   }

   /// this loop can be skiped for validate-only nodes as it is merely gathering stats for indicies
   if( db().has_hardfork( STEEMIT_HARDFORK_0_6__80 ) && comment.parent_author.size() != 0 )
   {
      auto parent = &db().get_comment( comment.parent_author, comment.parent_permlink );
      auto now = db().head_block_time();
      while( parent )
      {
         db().modify( *parent, [&]( comment_object& p ){
            p.children--;
            p.active = now;
         });
   #ifndef IS_LOW_MEM
         if( parent->parent_author.size() )
            parent = &db().get_comment( parent->parent_author, parent->parent_permlink );
         else
   #endif
            parent = nullptr;
      }
   }

   /** TODO move category behavior to a plugin, this is not part of consensus */
   const category_object* cat = db().find_category( comment.category );
   db().modify( *cat, [&]( category_object& c )
   {
      c.discussions--;
      c.last_update = db().head_block_time();
   });

   db().remove( comment );
}

void comment_options_evaluator::do_apply( const comment_options_operation& o )
{
   if( db().has_hardfork( STEEMIT_HARDFORK_0_10 ) )
   {
      const auto& auth = db().get_account( o.author );
      FC_ASSERT( !(auth.owner_challenged || auth.active_challenged ), "cannot process operation because account is currently challenged" );
   }


   const auto& comment = db().get_comment( o.author, o.permlink );
   if( !o.allow_curation_rewards || !o.allow_votes || o.max_accepted_payout < comment.max_accepted_payout )
      FC_ASSERT( comment.abs_rshares == 0, "operations contains options that are only updatedable on an account with no rshares" );

   FC_ASSERT( o.extensions.size() == 0, "extensions are not currently supported" );
   FC_ASSERT( comment.allow_curation_rewards >= o.allow_curation_rewards, "cannot re-enable curation rewards" );
   FC_ASSERT( comment.allow_votes >= o.allow_votes, "cannot re-enable votes" );
   FC_ASSERT( comment.max_accepted_payout >= o.max_accepted_payout, "cannot accept a greater payout" );
   FC_ASSERT( comment.percent_steem_dollars >= o.percent_steem_dollars, "cannot accept a greater percent Steem Dollars" );

   db().modify( comment, [&]( comment_object& c ) {
       c.max_accepted_payout   = o.max_accepted_payout;
       c.percent_steem_dollars = o.percent_steem_dollars;
       c.allow_votes           = o.allow_votes;
       c.allow_curation_rewards = o.allow_curation_rewards;
   });
}
void comment_evaluator::do_apply( const comment_operation& o )
{ try {
   if( db().is_producing() || db().has_hardfork( STEEMIT_HARDFORK_0_5__55 ) )
      FC_ASSERT( o.title.size() + o.body.size() + o.json_metadata.size(), "something should change" );

   const auto& by_permlink_idx = db().get_index_type< comment_index >().indices().get< by_permlink >();
   auto itr = by_permlink_idx.find( boost::make_tuple( o.author, o.permlink ) );

   const auto& auth = db().get_account( o.author ); /// prove it exists

   if( db().has_hardfork( STEEMIT_HARDFORK_0_10 ) )
      FC_ASSERT( !(auth.owner_challenged || auth.active_challenged ), "cannot process operation because account is currently challenged" );

   comment_id_type id;

   const comment_object* parent = nullptr;
   if( o.parent_author.size() != 0 ) {
      parent = &db().get_comment( o.parent_author, o.parent_permlink );
      FC_ASSERT( parent->depth < STEEMIT_MAX_COMMENT_DEPTH, "Comment is nested ${x} posts deep, maximum depth is ${y}", ("x",parent->depth)("y",STEEMIT_MAX_COMMENT_DEPTH) );
   }
   auto now = db().head_block_time();

   if ( itr == by_permlink_idx.end() )
   {
      if( o.parent_author.size() != 0 )
      {
         FC_ASSERT( parent->root_comment( db() ).allow_replies, "Comment has disabled replies." );
         if( db().has_hardfork( STEEMIT_HARDFORK_0_12__177) )
            FC_ASSERT( db().calculate_discussion_payout_time( *parent ) != fc::time_point_sec::maximum(), "discussion is frozen" );
      }

      if( db().has_hardfork( STEEMIT_HARDFORK_0_12__176 ) )
      {
         if( o.parent_author.size() == 0 )
             FC_ASSERT( (now - auth.last_root_post) > STEEMIT_MIN_ROOT_COMMENT_INTERVAL, "You may only post once every 5 minutes", ("now",now)("auth.last_root_post",auth.last_root_post) );
         else
             FC_ASSERT( (now - auth.last_post) > STEEMIT_MIN_REPLY_INTERVAL, "You may only comment once every 20 seconds", ("now",now)("auth.last_post",auth.last_post) );
      }
      else if( db().has_hardfork( STEEMIT_HARDFORK_0_6__113 ) )
      {
         if( o.parent_author.size() == 0 )
             FC_ASSERT( (now - auth.last_post) > STEEMIT_MIN_ROOT_COMMENT_INTERVAL, "You may only post once every 5 minutes", ("now",now)("auth.last_post",auth.last_post) );
         else
             FC_ASSERT( (now - auth.last_post) > STEEMIT_MIN_REPLY_INTERVAL, "You may only comment once every 20 seconds", ("now",now)("auth.last_post",auth.last_post) );
      }
      else
      {
         FC_ASSERT( (now - auth.last_post) > fc::seconds(60), "You may only post once per minute", ("now",now)("auth.last_post",auth.last_post) );
      }

      uint16_t reward_weight = STEEMIT_100_PERCENT;
      uint64_t post_bandwidth = auth.post_bandwidth;

      if( db().has_hardfork( STEEMIT_HARDFORK_0_12__176 ) && o.parent_author.size() == 0 )
      {
         uint64_t post_delta_time = std::min( db().head_block_time().sec_since_epoch() - auth.last_root_post.sec_since_epoch(), STEEMIT_POST_AVERAGE_WINDOW );
         uint32_t old_weight = uint32_t( ( post_bandwidth * ( STEEMIT_POST_AVERAGE_WINDOW - post_delta_time ) ) / STEEMIT_POST_AVERAGE_WINDOW );
         post_bandwidth = ( old_weight + STEEMIT_100_PERCENT );
         reward_weight = uint16_t( std::min( ( STEEMIT_POST_WEIGHT_CONSTANT * STEEMIT_100_PERCENT ) / ( post_bandwidth * post_bandwidth ), uint64_t( STEEMIT_100_PERCENT ) ) );
      }

      db().modify( auth, [&]( account_object& a ) {
         if( o.parent_author.size() == 0 )
         {
            a.last_root_post = now;
            a.post_bandwidth = uint32_t( post_bandwidth );
         }
         a.last_post = now;
         a.post_count++;
      });

      const auto& new_comment = db().create< comment_object >( [&]( comment_object& com )
      {
         if( db().has_hardfork( STEEMIT_HARDFORK_0_1 ) )
         {
            validate_permlink_0_1( o.parent_permlink );
            validate_permlink_0_1( o.permlink );
         }

         com.author = o.author;
         com.permlink = o.permlink;
         com.last_update = db().head_block_time();
         com.created = com.last_update;
         com.active = com.last_update;
         com.last_payout = fc::time_point_sec::min();
         com.max_cashout_time = fc::time_point_sec::maximum();
         com.reward_weight = reward_weight;

         if ( o.parent_author.size() == 0 )
         {
            com.parent_author = "";
            com.parent_permlink = o.parent_permlink;
            com.category = o.parent_permlink;
            com.root_comment = com.id;
            com.cashout_time = db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) ?
               db().head_block_time() + STEEMIT_CASHOUT_WINDOW_SECONDS :
               fc::time_point_sec::maximum();
         }
         else
         {
            com.parent_author = parent->author;
            com.parent_permlink = parent->permlink;
            com.depth = parent->depth + 1;
            com.category = parent->category;
            com.root_comment = parent->root_comment;
            com.cashout_time = fc::time_point_sec::maximum();
         }

         #ifndef IS_LOW_MEM
            com.title = o.title;
            if( o.body.size() < 1024*1024*128 )
            {
               com.body = o.body;
            }
            com.json_metadata = o.json_metadata;
         #endif
      });

      /** TODO move category behavior to a plugin, this is not part of consensus */
      const category_object* cat = db().find_category( new_comment.category );
      if( !cat ) {
         cat = &db().create<category_object>( [&]( category_object& c ){
             c.name = new_comment.category;
             c.discussions = 1;
             c.last_update = db().head_block_time();
         });
      } else {
         db().modify( *cat, [&]( category_object& c ){
             c.discussions++;
             c.last_update = db().head_block_time();
         });
      }

      id = new_comment.id;

/// this loop can be skiped for validate-only nodes as it is merely gathering stats for indicies
      auto now = db().head_block_time();
      while( parent ) {
         db().modify( *parent, [&]( comment_object& p ){
            p.children++;
            p.active = now;
         });
#ifndef IS_LOW_MEM
         if( parent->parent_author.size() )
            parent = &db().get_comment( parent->parent_author, parent->parent_permlink );
         else
#endif
            parent = nullptr;
      }

   }
   else // start edit case
   {
      const auto& comment = *itr;

      if( db().has_hardfork( STEEMIT_HARDFORK_0_14__306 ) )
         FC_ASSERT( comment.mode != archived, "comment is archived" );
      else if( db().has_hardfork( STEEMIT_HARDFORK_0_10 ) )
         FC_ASSERT( comment.last_payout == fc::time_point_sec::min(), "Can only edit during the first 24 hours" );

      db().modify( comment, [&]( comment_object& com )
      {
         com.last_update   = db().head_block_time();
         com.active        = com.last_update;

         if( !parent )
         {
            FC_ASSERT( com.parent_author == "", "The parent of a comment cannot change" );
            FC_ASSERT( com.parent_permlink == o.parent_permlink, "The permlink of a comment cannot change" );
         }
         else
         {
            FC_ASSERT( com.parent_author == o.parent_author, "The parent of a comment cannot change" );
            FC_ASSERT( com.parent_permlink == o.parent_permlink, "The permlink of a comment cannot change" );
         }

         #ifndef IS_LOW_MEM
           if( o.title.size() )         com.title         = o.title;
           if( o.json_metadata.size() ) com.json_metadata = o.json_metadata;

           if( o.body.size() ) {
              try {
               diff_match_patch<std::wstring> dmp;
               auto patch = dmp.patch_fromText( utf8_to_wstring(o.body) );
               if( patch.size() ) {
                  auto result = dmp.patch_apply( patch, utf8_to_wstring(com.body) );
                  auto patched_body = wstring_to_utf8(result.first);
                  if( !fc::is_utf8( patched_body ) ) {
                     idump(("invalid utf8")(patched_body));
                     com.body = fc::prune_invalid_utf8(patched_body);
                  } else { com.body = patched_body; }
               }
               else { // replace
                  com.body = o.body;
               }
              } catch ( ... ) {
                  com.body = o.body;
              }
           }
         #endif

      });

   } // end EDIT case

} FC_CAPTURE_AND_RETHROW( (o) ) }

void escrow_transfer_evaluator::do_apply( const escrow_transfer_operation& o )
{
   try
   {
      FC_ASSERT( db().has_hardfork( STEEMIT_HARDFORK_0_14__143 ), "op is not valid until next hardfork" ); /// TODO: remove this after HF14

      const auto& from_account = db().get_account(o.from);
      db().get_account(o.to);
      db().get_account(o.agent);

      FC_ASSERT( o.ratification_deadline > db().head_block_time(), "ratification deadline must be after head block time" );
      FC_ASSERT( o.escrow_expiration > db().head_block_time(), "escrow expiration must be after head block time" );

      if( o.fee.symbol == STEEM_SYMBOL )
      {
         FC_ASSERT( from_account.balance >= o.steem_amount + o.fee, "account cannot cover steem costs of escrow" );
         FC_ASSERT( from_account.sbd_balance >= o.sbd_amount, "account cannot cover sbd costs of escrow" );
      }
      else
      {
         FC_ASSERT( from_account.balance >= o.steem_amount, "account cannot cover steem costs of escrow" );
         FC_ASSERT( from_account.sbd_balance >= o.sbd_amount + o.fee, "account cannot cover sbd costs of escrow" );
      }

      if( o.fee.amount > 0 )
      {
         db().adjust_balance( from_account, -o.fee );
      }

      db().adjust_balance( from_account, -o.steem_amount );
      db().adjust_balance( from_account, -o.sbd_amount );

      db().create<escrow_object>([&]( escrow_object& esc )
      {
         esc.escrow_id              = o.escrow_id;
         esc.from                   = o.from;
         esc.to                     = o.to;
         esc.agent                  = o.agent;
         esc.sbd_balance            = o.sbd_amount;
         esc.steem_balance          = o.steem_amount;
         esc.pending_fee            = o.fee;
         esc.ratification_deadline  = o.ratification_deadline;
         esc.escrow_expiration      = o.escrow_expiration;
      });
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_approve_evaluator::do_apply( const escrow_approve_operation& o )
{
   try
   {
      FC_ASSERT( db().has_hardfork( STEEMIT_HARDFORK_0_14__143 ), "op is not valid until next hardfork" ); /// TODO: remove this after HF14

      const auto& escrow = db().get_escrow( o.from, o.escrow_id );

      FC_ASSERT( escrow.to == o.to, "op 'to' does not match escrow 'to'" );
      FC_ASSERT( escrow.agent == o.agent, "op 'agent' does not match escrow 'agent'" );

      bool reject_escrow = !o.approve;

      if( o.who == o.to )
      {
         FC_ASSERT( !escrow.to_approved, "'to' has already approved the escrow" );

         if( !reject_escrow )
         {
            db().modify( escrow, [&]( escrow_object& esc )
            {
               esc.to_approved = true;
            });
         }
      }
      if( o.who == o.agent )
      {
         FC_ASSERT( !escrow.agent_approved, "'agent' has already approved the escrow" );

         if( !reject_escrow )
         {
            db().modify( escrow, [&]( escrow_object& esc )
            {
               esc.agent_approved = true;
            });
         }
      }

      if( reject_escrow )
      {
         const auto& from_account = db().get_account( o.from );
         db().adjust_balance( from_account, escrow.steem_balance );
         db().adjust_balance( from_account, escrow.sbd_balance );
         db().adjust_balance( from_account, escrow.pending_fee );

         db().remove( escrow );
      }
      else if( escrow.to_approved && escrow.agent_approved )
      {
         const auto& agent_account = db().get_account( o.agent );
         db().adjust_balance( agent_account, escrow.pending_fee );

         db().modify( escrow, [&]( escrow_object& esc )
         {
            esc.pending_fee.amount = 0;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_dispute_evaluator::do_apply( const escrow_dispute_operation& o )
{
   try
   {
      FC_ASSERT( db().has_hardfork( STEEMIT_HARDFORK_0_14__143 ), "op is not valid until next hardfork" ); /// TODO: remove this after HF14
      const auto& from_account = db().get_account( o.from );

      const auto& e = db().get_escrow( o.from, o.escrow_id );
      FC_ASSERT( db().head_block_time() < e.escrow_expiration, "disputes must be raised before expiration" );
      FC_ASSERT( e.to_approved && e.agent_approved, "escrow must be approved by all parties before a dispute can be raised" );
      FC_ASSERT( !e.disputed, "escrow is already under dispute" );
      FC_ASSERT( e.to == o.to, "op 'to' does not match escrow 'to'");

      db().modify( e, [&]( escrow_object& esc )
      {
         esc.disputed = true;
      });
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_release_evaluator::do_apply( const escrow_release_operation& o )
{
   try
   {
      FC_ASSERT( db().has_hardfork( STEEMIT_HARDFORK_0_14__143 ), "op is not valid until next hardfork" ); /// TODO: remove this after HF14

      const auto& from_account = db().get_account(o.from);
      const auto& to_account = db().get_account(o.to);
      const auto& who_account = db().get_account(o.who);

      const auto& e = db().get_escrow( o.from, o.escrow_id );
      FC_ASSERT( e.steem_balance >= o.steem_amount, "Release amount exceeds escrow balance" );
      FC_ASSERT( e.sbd_balance >= o.sbd_amount, "Release amount exceeds escrow balance" );
      FC_ASSERT( o.to == e.from || o.to == e.to, "Funds must be released to 'from' or 'to'" );
      FC_ASSERT( e.to_approved && e.agent_approved, "Funds cannot be released prior to escrow approval" );

      // If there is a dispute regardless of expiration, the agent can release funds to either party
      if( e.disputed )
      {
         FC_ASSERT( o.who == e.agent, "'agent' must release funds for a disputed escrow" );
      }
      else
      {
         FC_ASSERT( o.who == e.from || o.who == e.to, "Only 'from' and 'to' can release from a non-disputed escrow" );

         if( e.escrow_expiration > db().head_block_time() )
         {
            // If there is no dispute and escrow has not expired, either party can release funds to the other.
            if( o.who == e.from )
            {
               FC_ASSERT( o.to == e.to, "'from' must release funds to 'to'" );
            }
            else if( o.who == e.to )
            {
               FC_ASSERT( o.to == e.from, "'to' must release funds to 'from'" );
            }
         }
      }
      // If escrow expires and there is no dispute, either party can release funds to either party.

      db().adjust_balance( to_account, o.steem_amount );
      db().adjust_balance( to_account, o.sbd_amount );

      db().modify( e, [&]( escrow_object& esc )
      {
         esc.steem_balance -= o.steem_amount;
         esc.sbd_balance -= o.sbd_amount;
      });

      if( e.steem_balance.amount == 0 && e.sbd_balance.amount == 0 )
      {
         db().remove( e );
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void transfer_evaluator::do_apply( const transfer_operation& o )
{
   const auto& from_account = db().get_account(o.from);
   const auto& to_account = db().get_account(o.to);

   if( from_account.active_challenged )
   {
      db().modify( from_account, [&]( account_object& a )
      {
         a.active_challenged = false;
         a.last_active_proved = db().head_block_time();
      });
   }

   FC_ASSERT( o.amount.symbol != VESTS_SYMBOL, "transferring of Steem Power (STMP) is not allowed." );

   FC_ASSERT( db().get_balance( from_account, o.amount.symbol ) >= o.amount, "account does not have sufficient funds for transfer" );
   db().adjust_balance( from_account, -o.amount );
   db().adjust_balance( to_account, o.amount );
}

void transfer_to_vesting_evaluator::do_apply( const transfer_to_vesting_operation& o )
{
   const auto& from_account = db().get_account(o.from);
   const auto& to_account = o.to.size() ? db().get_account(o.to) : from_account;

   FC_ASSERT( db().get_balance( from_account, STEEM_SYMBOL) >= o.amount, "account does not have sufficient STEEM for the transfer" );
   db().adjust_balance( from_account, -o.amount );
   db().create_vesting( to_account, o.amount );
}

void withdraw_vesting_evaluator::do_apply( const withdraw_vesting_operation& o )
{
    const auto& account = db().get_account( o.account );

    FC_ASSERT( account.vesting_shares >= asset( 0, VESTS_SYMBOL ), "account does not have sufficient Steem Power for withdraw" );
    FC_ASSERT( account.vesting_shares >= o.vesting_shares, "account does not have sufficient Steem Power for withdraw" );

    if( !account.mined && db().has_hardfork( STEEMIT_HARDFORK_0_1 ) ) {
      const auto& props = db().get_dynamic_global_properties();
      const witness_schedule_object& wso = db().get_witness_schedule_object();

      asset min_vests = wso.median_props.account_creation_fee * props.get_vesting_share_price();
      min_vests.amount.value *= 10;

      FC_ASSERT( account.vesting_shares > min_vests,
                 "Account registered by another account requires 10x account creation fee worth of Steem Power before it can power down" );
    }



    if( o.vesting_shares.amount == 0 ) {
       if( db().is_producing() || db().has_hardfork( STEEMIT_HARDFORK_0_5__57 ) )
          FC_ASSERT( account.vesting_withdraw_rate.amount  != 0, "this operation would not change the vesting withdraw rate" );

       db().modify( account, [&]( account_object& a ) {
         a.vesting_withdraw_rate = asset( 0, VESTS_SYMBOL );
         a.next_vesting_withdrawal = time_point_sec::maximum();
         a.to_withdraw = 0;
         a.withdrawn = 0;
       });
    }
    else {
       db().modify( account, [&]( account_object& a ) {
         auto new_vesting_withdraw_rate = asset( o.vesting_shares.amount / STEEMIT_VESTING_WITHDRAW_INTERVALS, VESTS_SYMBOL );

         if( new_vesting_withdraw_rate.amount == 0 )
            new_vesting_withdraw_rate.amount = 1;

         if( db().is_producing() || db().has_hardfork( STEEMIT_HARDFORK_0_5__57 ) )
            FC_ASSERT( account.vesting_withdraw_rate  != new_vesting_withdraw_rate, "this operation would not change the vesting withdraw rate" );

         a.vesting_withdraw_rate = new_vesting_withdraw_rate;

         a.next_vesting_withdrawal = db().head_block_time() + fc::seconds(STEEMIT_VESTING_WITHDRAW_INTERVAL_SECONDS);
         a.to_withdraw = o.vesting_shares.amount;
         a.withdrawn = 0;
       });
    }
}

void set_withdraw_vesting_route_evaluator::do_apply( const set_withdraw_vesting_route_operation& o )
{
   try
   {
   const auto& from_account = db().get_account( o.from_account );
   const auto& to_account = db().get_account( o.to_account );
   const auto& wd_idx = db().get_index_type< withdraw_vesting_route_index >().indices().get< by_withdraw_route >();
   auto itr = wd_idx.find( boost::make_tuple( from_account.id, to_account.id ) );

   if( itr == wd_idx.end() )
   {
      FC_ASSERT( o.percent != 0, "Cannot create a 0% destination." );
      FC_ASSERT( from_account.withdraw_routes < STEEMIT_MAX_WITHDRAW_ROUTES, "account already has the maximum number of routes" );

      db().create< withdraw_vesting_route_object >( [&]( withdraw_vesting_route_object& wvdo )
      {
         wvdo.from_account = from_account.id;
         wvdo.to_account = to_account.id;
         wvdo.percent = o.percent;
         wvdo.auto_vest = o.auto_vest;
      });

      db().modify( from_account, [&]( account_object& a )
      {
         a.withdraw_routes++;
      });
   }
   else if( o.percent == 0 )
   {
      db().remove( *itr );

      db().modify( from_account, [&]( account_object& a )
      {
         a.withdraw_routes--;
      });
   }
   else
   {
      db().modify( *itr, [&]( withdraw_vesting_route_object& wvdo )
      {
         wvdo.from_account = from_account.id;
         wvdo.to_account = to_account.id;
         wvdo.percent = o.percent;
         wvdo.auto_vest = o.auto_vest;
      });
   }

   itr = wd_idx.upper_bound( boost::make_tuple( from_account.id, account_id_type() ) );
   uint16_t total_percent = 0;

   while( itr->from_account == from_account.id && itr != wd_idx.end() )
   {
      total_percent += itr->percent;
      ++itr;
   }

   FC_ASSERT( total_percent <= STEEMIT_100_PERCENT, "More than 100% of vesting allocated to destinations" );
   }
   FC_CAPTURE_AND_RETHROW()
}

void account_witness_proxy_evaluator::do_apply( const account_witness_proxy_operation& o )
{
   const auto& account = db().get_account( o.account );
   FC_ASSERT( account.proxy != o.proxy, "something must change" );

   /// remove all current votes
   std::array<share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1> delta;
   delta[0] = -account.vesting_shares.amount;
   for( int i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
      delta[i+1] = -account.proxied_vsf_votes[i];
   db().adjust_proxied_witness_votes( account, delta );

   if( o.proxy.size() ) {
      const auto& new_proxy = db().get_account( o.proxy );
      flat_set<account_id_type> proxy_chain({account.get_id(), new_proxy.get_id()});
      proxy_chain.reserve( STEEMIT_MAX_PROXY_RECURSION_DEPTH + 1 );

      /// check for proxy loops and fail to update the proxy if it would create a loop
      auto cprox = &new_proxy;
      while( cprox->proxy.size() != 0 ) {
         const auto next_proxy = db().get_account( cprox->proxy );
         FC_ASSERT( proxy_chain.insert( next_proxy.get_id() ).second, "Attempt to create a proxy loop" );
         cprox = &next_proxy;
         FC_ASSERT( proxy_chain.size() <= STEEMIT_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long" );
      }

      /// clear all individual vote records
      db().clear_witness_votes( account );

      db().modify( account, [&]( account_object& a ) {
         a.proxy = o.proxy;
      });

      /// add all new votes
      for( int i = 0; i <= STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
         delta[i] = -delta[i];
      db().adjust_proxied_witness_votes( account, delta );
   } else { /// we are clearing the proxy which means we simply update the account
      db().modify( account, [&]( account_object& a ) {
          a.proxy = o.proxy;
      });
   }
}


void account_witness_vote_evaluator::do_apply( const account_witness_vote_operation& o )
{
   const auto& voter = db().get_account( o.account );
   FC_ASSERT( voter.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting for a witness" );

   const auto& witness = db().get_witness( o.witness );

   const auto& by_account_witness_idx = db().get_index_type< witness_vote_index >().indices().get< by_account_witness >();
   auto itr = by_account_witness_idx.find( boost::make_tuple( voter.get_id(), witness.get_id() ) );

   if( itr == by_account_witness_idx.end() ) {
      FC_ASSERT( o.approve, "vote doesn't exist, user must be indicate a desire to approve witness" );

      if ( db().has_hardfork( STEEMIT_HARDFORK_0_2 ) )
      {
         FC_ASSERT( voter.witnesses_voted_for < STEEMIT_MAX_ACCOUNT_WITNESS_VOTES, "account has voted for too many witnesses" ); // TODO: Remove after hardfork 2

         db().create<witness_vote_object>( [&]( witness_vote_object& v ) {
             v.witness = witness.id;
             v.account = voter.id;
         });

         if( db().has_hardfork( STEEMIT_HARDFORK_0_3 ) ) {
            db().adjust_witness_vote( witness, voter.witness_vote_weight() );
         }
         else {
            db().adjust_proxied_witness_votes( voter, voter.witness_vote_weight() );
         }

      } else {

         db().create<witness_vote_object>( [&]( witness_vote_object& v ) {
             v.witness = witness.id;
             v.account = voter.id;
         });
         db().modify( witness, [&]( witness_object& w ) {
             w.votes += voter.witness_vote_weight();
         });

      }
      db().modify( voter, [&]( account_object& a ) {
         a.witnesses_voted_for++;
      });

   } else {
      FC_ASSERT( !o.approve, "vote currently exists, user must be indicate a desire to reject witness" );

      if (  db().has_hardfork( STEEMIT_HARDFORK_0_2 ) ) {
         if( db().has_hardfork( STEEMIT_HARDFORK_0_3 ) )
            db().adjust_witness_vote( witness, -voter.witness_vote_weight() );
         else
            db().adjust_proxied_witness_votes( voter, -voter.witness_vote_weight() );
      } else  {
         db().modify( witness, [&]( witness_object& w ) {
             w.votes -= voter.witness_vote_weight();
         });
      }
      db().modify( voter, [&]( account_object& a ) {
         a.witnesses_voted_for--;
      });
      db().remove( *itr );
   }
}

void vote_evaluator::do_apply( const vote_operation& o )
{ try {

   const auto& comment = db().get_comment( o.author, o.permlink );
   const auto& voter   = db().get_account( o.voter );

   if( db().has_hardfork( STEEMIT_HARDFORK_0_10 ) )
      FC_ASSERT( !(voter.owner_challenged || voter.active_challenged ), "Account is currently challenged" );

   if( o.weight > 0 ) FC_ASSERT( comment.allow_votes, "Votes are not allowed on the comment" );

   if( db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) && db().calculate_discussion_payout_time( comment ) == fc::time_point_sec::maximum() )
   {
#ifndef CLEAR_VOTES
      const auto& comment_vote_idx = db().get_index_type< comment_vote_index >().indices().get< by_comment_voter >();
      auto itr = comment_vote_idx.find( std::make_tuple( comment.id, voter.id ) );

      if( itr == comment_vote_idx.end() )
         db().create< comment_vote_object >( [&]( comment_vote_object& cvo )
         {
            cvo.voter = voter.id;
            cvo.comment = comment.id;
            cvo.vote_percent = o.weight;
            cvo.last_update = db().head_block_time();
         });
      else
         db().modify( *itr, [&]( comment_vote_object& cvo )
         {
            cvo.vote_percent = o.weight;
            cvo.last_update = db().head_block_time();
         });
#endif
      return;
   }

   const auto& comment_vote_idx = db().get_index_type< comment_vote_index >().indices().get< by_comment_voter >();
   auto itr = comment_vote_idx.find( std::make_tuple( comment.id, voter.id ) );

   auto elapsed_seconds   = (db().head_block_time() - voter.last_vote_time).to_seconds();

   if( db().has_hardfork( STEEMIT_HARDFORK_0_11 ) )
      FC_ASSERT( elapsed_seconds >= STEEMIT_MIN_VOTE_INTERVAL_SEC, "Can only vote once every 3 seconds" );

   auto regenerated_power = (STEEMIT_100_PERCENT * elapsed_seconds) /  STEEMIT_VOTE_REGENERATION_SECONDS;
   auto current_power     = std::min( int64_t(voter.voting_power + regenerated_power), int64_t(STEEMIT_100_PERCENT) );
   FC_ASSERT( current_power > 0, "Account currently does not have voting power" );

   int64_t  abs_weight    = abs(o.weight);
   auto     used_power    = (current_power * abs_weight) / STEEMIT_100_PERCENT;
   used_power = (used_power/200);
   if( !db().has_hardfork( STEEMIT_HARDFORK_0_14__259 ) )
      used_power += 1;
   FC_ASSERT( used_power <= current_power, "Account does not have enough power for vote" );

   int64_t abs_rshares    = ((uint128_t(voter.vesting_shares.amount.value) * used_power) / (STEEMIT_100_PERCENT)).to_uint64();
   if( !db().has_hardfork( STEEMIT_HARDFORK_0_14__259 ) && abs_rshares == 0 ) abs_rshares = 1;

   if( db().is_producing() || db().has_hardfork( STEEMIT_HARDFORK_0_13__248 ) ) {
      FC_ASSERT( abs_rshares > 50000000 || o.weight == 0, "voting weight is too small, please accumulate more voting power or steem power" );
   }



   // Lazily delete vote
   if( itr != comment_vote_idx.end() && itr->num_changes == -1 )
   {
      if( db().is_producing() || db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) )
         FC_ASSERT( false, "Cannot vote again on a comment after payout" );

      db().remove( *itr );
      itr = comment_vote_idx.end();
   }

   if( itr == comment_vote_idx.end() )
   {
      FC_ASSERT( o.weight != 0, "Vote weight cannot be 0" );
      /// this is the rshares voting for or against the post
      int64_t rshares        = o.weight < 0 ? -abs_rshares : abs_rshares;

      if( rshares > 0 && db().has_hardfork( STEEMIT_HARDFORK_0_7 ) )
      {
         FC_ASSERT( db().head_block_time() < db().calculate_discussion_payout_time( comment ) - STEEMIT_UPVOTE_LOCKOUT, "Cannot increase reward of post within the last minute before payout" );
      }

      //used_power /= (50*7); /// a 100% vote means use .28% of voting power which should force users to spread their votes around over 50+ posts day for a week
      //if( used_power == 0 ) used_power = 1;

      db().modify( voter, [&]( account_object& a ){
         a.voting_power = current_power - used_power;
         a.last_vote_time = db().head_block_time();
      });

      /// if the current net_rshares is less than 0, the post is getting 0 rewards so it is not factored into total rshares^2
      fc::uint128_t old_rshares = std::max(comment.net_rshares.value, int64_t(0));
      const auto& root = comment.root_comment( db() );
      auto old_root_abs_rshares = root.children_abs_rshares.value;

      fc::uint128_t cur_cashout_time_sec = db().calculate_discussion_payout_time( comment ).sec_since_epoch();
      fc::uint128_t new_cashout_time_sec;

      if( db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) && !db().has_hardfork( STEEMIT_HARDFORK_0_13__257)  )
         new_cashout_time_sec = db().head_block_time().sec_since_epoch() + STEEMIT_CASHOUT_WINDOW_SECONDS;
      else
         new_cashout_time_sec = db().head_block_time().sec_since_epoch() + STEEMIT_CASHOUT_WINDOW_SECONDS_PRE_HF12;

      auto avg_cashout_sec = ( cur_cashout_time_sec * old_root_abs_rshares + new_cashout_time_sec * abs_rshares ) / ( old_root_abs_rshares + abs_rshares );

      FC_ASSERT( abs_rshares > 0, "Cannot vote with no rshares" );

      auto old_vote_rshares = comment.vote_rshares;

      db().modify( comment, [&]( comment_object& c ){
         c.net_rshares += rshares;
         c.abs_rshares += abs_rshares;
         if( rshares > 0 )
            c.vote_rshares += rshares;
         if( rshares > 0 )
            c.net_votes++;
         else
            c.net_votes--;
         if( !db().has_hardfork( STEEMIT_HARDFORK_0_6__114 ) && c.net_rshares == -c.abs_rshares) FC_ASSERT( c.net_votes < 0, "Comment has negative net votes?" );
      });

      db().modify( root, [&]( comment_object& c )
      {
         c.children_abs_rshares += abs_rshares;
         if( db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) && c.last_payout > fc::time_point_sec::min() )
            c.cashout_time = c.last_payout + STEEMIT_SECOND_CASHOUT_WINDOW;
         else
            c.cashout_time = fc::time_point_sec( std::min( uint32_t( avg_cashout_sec.to_uint64() ), c.max_cashout_time.sec_since_epoch() ) );

         if( c.max_cashout_time == fc::time_point_sec::maximum() )
            c.max_cashout_time = db().head_block_time() + fc::seconds( STEEMIT_MAX_CASHOUT_WINDOW_SECONDS );
      });

      fc::uint128_t new_rshares = std::max( comment.net_rshares.value, int64_t(0));

      /// calculate rshares2 value
      new_rshares = db().calculate_vshares( new_rshares );
      old_rshares = db().calculate_vshares( old_rshares );

      const auto& cat = db().get_category( comment.category );
      db().modify( cat, [&]( category_object& c ){
         c.abs_rshares += abs_rshares;
         c.last_update = db().head_block_time();
      });

      uint64_t max_vote_weight = 0;

      /** this verifies uniqueness of voter
       *
       *  cv.weight / c.total_vote_weight ==> % of rshares increase that is accounted for by the vote
       *
       *  W(R) = B * R / ( R + 2S )
       *  W(R) is bounded above by B. B is fixed at 2^64 - 1, so all weights fit in a 64 bit integer.
       *
       *  The equation for an individual vote is:
       *    W(R_N) - W(R_N-1), which is the delta increase of proportional weight
       *
       *  c.total_vote_weight =
       *    W(R_1) - W(R_0) +
       *    W(R_2) - W(R_1) + ...
       *    W(R_N) - W(R_N-1) = W(R_N) - W(R_0)
       *
       *  Since W(R_0) = 0, c.total_vote_weight is also bounded above by B and will always fit in a 64 bit integer.
       *
      **/
      const auto& cvo = db().create<comment_vote_object>( [&]( comment_vote_object& cv ){
         cv.voter   = voter.id;
         cv.comment = comment.id;
         cv.rshares = rshares;
         cv.vote_percent = o.weight;
         cv.last_update = db().head_block_time();

         if( rshares > 0 && (comment.last_payout == fc::time_point_sec())  && comment.allow_curation_rewards )
         {
            if( comment.created < fc::time_point_sec(STEEMIT_HARDFORK_0_6_REVERSE_AUCTION_TIME) ) {
               u512 rshares3(rshares);
               u256 total2( comment.abs_rshares.value );

               if( !db().has_hardfork( STEEMIT_HARDFORK_0_1 ) )
               {
                  rshares3 *= 1000000;
                  total2 *= 1000000;
               }

               rshares3 = rshares3 * rshares3 * rshares3;

               total2 *= total2;
               cv.weight = static_cast<uint64_t>( rshares3 / total2 );
            } else {// cv.weight = W(R_1) - W(R_0)
               if( db().has_hardfork( STEEMIT_HARDFORK_0_1 ) )
               {
                  uint64_t old_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( old_vote_rshares.value ) ) / ( 2 * db().get_content_constant_s() + old_vote_rshares.value ) ).to_uint64();
                  uint64_t new_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( comment.vote_rshares.value ) ) / ( 2 * db().get_content_constant_s() + comment.vote_rshares.value ) ).to_uint64();
                  cv.weight = new_weight - old_weight;
               }
               else
               {
                  uint64_t old_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( 1000000 * old_vote_rshares.value ) ) / ( 2 * db().get_content_constant_s() + ( 1000000 * old_vote_rshares.value ) ) ).to_uint64();
                  uint64_t new_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( 1000000 * comment.vote_rshares.value ) ) / ( 2 * db().get_content_constant_s() + ( 1000000 * comment.vote_rshares.value ) ) ).to_uint64();
                  cv.weight = new_weight - old_weight;
               }
            }

            max_vote_weight = cv.weight;

            if( db().head_block_time() > fc::time_point_sec(STEEMIT_HARDFORK_0_6_REVERSE_AUCTION_TIME) )  /// start enforcing this prior to the hardfork
            {
               /// discount weight by time
               uint128_t w(max_vote_weight);
               uint64_t delta_t = std::min( uint64_t((cv.last_update - comment.created).to_seconds()), uint64_t(STEEMIT_REVERSE_AUCTION_WINDOW_SECONDS) );

               w *= delta_t;
               w /= STEEMIT_REVERSE_AUCTION_WINDOW_SECONDS;
               cv.weight = w.to_uint64();
            }
         }
         else
         {
            cv.weight = 0;
         }
      });

      if( max_vote_weight ) // Optimization
      {
         db().modify( comment, [&]( comment_object& c )
         {
            c.total_vote_weight += max_vote_weight;
         });
      }

      db().adjust_rshares2( comment, old_rshares, new_rshares );
   }
   else
   {
      FC_ASSERT( itr->num_changes < STEEMIT_MAX_VOTE_CHANGES, "Cannot change vote again" );

      if( db().is_producing() || db().has_hardfork( STEEMIT_HARDFORK_0_6__112 ) )
         FC_ASSERT( itr->vote_percent != o.weight, "Changing your vote requires actually changing you vote." );

      /// this is the rshares voting for or against the post
      int64_t rshares        = o.weight < 0 ? -abs_rshares : abs_rshares;

      if( itr->rshares < rshares && db().has_hardfork( STEEMIT_HARDFORK_0_7 ) )
         FC_ASSERT( db().head_block_time() < db().calculate_discussion_payout_time( comment ) - STEEMIT_UPVOTE_LOCKOUT, "Cannot increase payout withing last minute before payout" );

      db().modify( voter, [&]( account_object& a ){
         a.voting_power = current_power - used_power;
         a.last_vote_time = db().head_block_time();
      });

      /// if the current net_rshares is less than 0, the post is getting 0 rewards so it is not factored into total rshares^2
      fc::uint128_t old_rshares = std::max(comment.net_rshares.value, int64_t(0));
      const auto& root = comment.root_comment( db() );
      auto old_root_abs_rshares = root.children_abs_rshares.value;

      fc::uint128_t cur_cashout_time_sec = db().calculate_discussion_payout_time( comment ).sec_since_epoch();
      fc::uint128_t new_cashout_time_sec;

      if( db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) && ! db().has_hardfork( STEEMIT_HARDFORK_0_13__257 )  )
         new_cashout_time_sec = db().head_block_time().sec_since_epoch() + STEEMIT_CASHOUT_WINDOW_SECONDS;
      else
         new_cashout_time_sec = db().head_block_time().sec_since_epoch() + STEEMIT_CASHOUT_WINDOW_SECONDS_PRE_HF12;

      fc::uint128_t avg_cashout_sec;
      if( db().has_hardfork( STEEMIT_HARDFORK_0_14__259 ) && abs_rshares == 0 )
         avg_cashout_sec = cur_cashout_time_sec;
      else
         avg_cashout_sec = ( cur_cashout_time_sec * old_root_abs_rshares + new_cashout_time_sec * abs_rshares ) / ( old_root_abs_rshares + abs_rshares );

      db().modify( comment, [&]( comment_object& c )
      {
         c.net_rshares -= itr->rshares;
         c.net_rshares += rshares;
         c.abs_rshares += abs_rshares;

         /// TODO: figure out how to handle remove a vote (rshares == 0 )
         if( rshares > 0 && itr->rshares < 0 )
            c.net_votes += 2;
         else if( rshares > 0 && itr->rshares == 0 )
            c.net_votes += 1;
         else if( rshares == 0 && itr->rshares < 0 )
            c.net_votes += 1;
         else if( rshares == 0 && itr->rshares > 0 )
            c.net_votes -= 1;
         else if( rshares < 0 && itr->rshares == 0 )
            c.net_votes -= 1;
         else if( rshares < 0 && itr->rshares > 0 )
            c.net_votes -= 2;
      });

      db().modify( root, [&]( comment_object& c )
      {
         c.children_abs_rshares += abs_rshares;
         if( db().has_hardfork( STEEMIT_HARDFORK_0_12__177 ) && c.last_payout > fc::time_point_sec::min() )
            c.cashout_time = c.last_payout + STEEMIT_SECOND_CASHOUT_WINDOW;
         else
            c.cashout_time = fc::time_point_sec( std::min( uint32_t( avg_cashout_sec.to_uint64() ), c.max_cashout_time.sec_since_epoch() ) );

         if( c.max_cashout_time == fc::time_point_sec::maximum() )
            c.max_cashout_time = db().head_block_time() + fc::seconds( STEEMIT_MAX_CASHOUT_WINDOW_SECONDS );
      });

      fc::uint128_t new_rshares = std::max( comment.net_rshares.value, int64_t(0));

      /// calculate rshares2 value
      new_rshares = db().calculate_vshares( new_rshares );
      old_rshares = db().calculate_vshares( old_rshares );

      db().modify( comment, [&]( comment_object& c )
      {
         c.total_vote_weight -= itr->weight;
      });

      db().modify( *itr, [&]( comment_vote_object& cv )
      {
         cv.rshares = rshares;
         cv.vote_percent = o.weight;
         cv.last_update = db().head_block_time();
         cv.weight = 0;
         cv.num_changes += 1;
      });

      db().adjust_rshares2( comment, old_rshares, new_rshares );
   }

} FC_CAPTURE_AND_RETHROW( (o)) }

void custom_evaluator::do_apply( const custom_operation& o ){}

void custom_json_evaluator::do_apply( const custom_json_operation& o )
{
   database& d = db();
   std::shared_ptr< generic_json_evaluator_registry > eval = d.get_custom_json_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      //elog( "Caught exception processing custom_json_operation:\n${e}", ("e", e.to_detail_string()) );
   }
   catch(...)
   {
      elog( "unexpected exception applying custom json evaluator" );
   }
}


void custom_binary_evaluator::do_apply( const custom_binary_operation& o )
{
   FC_ASSERT( db().has_hardfork( STEEMIT_HARDFORK_0_14__317 ) );
   /*
   database& d = db();
   std::shared_ptr< generic_json_evaluator_registry > eval = d.get_custom_json_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      //elog( "Caught exception processing custom_json_operation:\n${e}", ("e", e.to_detail_string()) );
   }
   catch(...)
   {
      elog( "unexpected exception applying custom json evaluator" );
   }
   */
}


template<typename Operation>
void pow_apply( database& db, Operation o ) {
   const auto& dgp = db.get_dynamic_global_properties();

   if( db.is_producing() || db.has_hardfork( STEEMIT_HARDFORK_0_5__59 ) )
   {
      const auto& witness_by_work = db.get_index_type<witness_index>().indices().get<by_work>();
      auto work_itr = witness_by_work.find( o.work.work );
      if( work_itr != witness_by_work.end() )
      {
          FC_ASSERT( !"DUPLICATE WORK DISCOVERED", "${w}  ${witness}",("w",o)("wit",*work_itr) );
      }
   }

   const auto& accounts_by_name = db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find(o.get_worker_account());
   if(itr == accounts_by_name.end()) {
      db.create< account_object >( [&]( account_object& acc )
      {
         acc.name = o.get_worker_account();
         acc.owner = authority( 1, o.work.worker, 1);
         acc.active = acc.owner;
         acc.posting = acc.owner;
         acc.memo_key = o.work.worker;
         acc.created = dgp.time;
         acc.last_vote_time = dgp.time;

         if( !db.has_hardfork( STEEMIT_HARDFORK_0_11__169 ) )
            acc.recovery_account = "steem";
         else
            acc.recovery_account = ""; /// highest voted witness at time of recovery
      });
   }

   const auto& worker_account = db.get_account( o.get_worker_account() ); // verify it exists
   FC_ASSERT( worker_account.active.num_auths() == 1, "miners can only have one key auth" );
   FC_ASSERT( worker_account.active.key_auths.size() == 1, "miners may only have one key auth" );
   FC_ASSERT( worker_account.active.key_auths.begin()->first == o.work.worker, "work must be performed by key that signed the work" );
   FC_ASSERT( o.block_id == db.head_block_id(), "pow not for last block" );
   if( db.has_hardfork( STEEMIT_HARDFORK_0_13__256 ) )
      FC_ASSERT( worker_account.last_account_update < db.head_block_time(), "worker account must not have updated their account this block" );

   fc::sha256 target = db.get_pow_target();

   FC_ASSERT( o.work.work < target, "work lacks sufficient difficulty" );

   db.modify( dgp, [&]( dynamic_global_property_object& p )
   {
      p.total_pow++; // make sure this doesn't break anything...
      p.num_pow_witnesses++;
   });


   const witness_object* cur_witness = db.find_witness( worker_account.name );
   if( cur_witness ) {
      FC_ASSERT( cur_witness->pow_worker == 0, "this account is already scheduled for pow block production" );
      db.modify(*cur_witness, [&]( witness_object& w ){
          w.props             = o.props;
          w.pow_worker        = dgp.total_pow;
          w.last_work         = o.work.work;
      });
   } else {
      db.create<witness_object>( [&]( witness_object& w )
      {
          w.owner             = o.get_worker_account();
          w.props             = o.props;
          w.signing_key       = o.work.worker;
          w.pow_worker        = dgp.total_pow;
          w.last_work         = o.work.work;
      });
   }
   /// POW reward depends upon whether we are before or after MINER_VOTING kicks in
   asset pow_reward = db.get_pow_reward();
   if( db.head_block_num() < STEEMIT_START_MINER_VOTING_BLOCK )
      pow_reward.amount *= STEEMIT_MAX_MINERS;
   db.adjust_supply( pow_reward, true );

   /// pay the witness that includes this POW
   const auto& inc_witness = db.get_account( dgp.current_witness );
   if( db.head_block_num() < STEEMIT_START_MINER_VOTING_BLOCK )
      db.adjust_balance( inc_witness, pow_reward );
   else
      db.create_vesting( inc_witness, pow_reward );
}

void pow_evaluator::do_apply( const pow_operation& o ) {
   FC_ASSERT( !db().has_hardfork( STEEMIT_HARDFORK_0_13__256 ), "pow is deprecated. Use pow2 instead" );
   pow_apply( db(), o );
}


void pow2_evaluator::do_apply( const pow2_operation& o ) {
   const auto& work = o.work.get<pow2>();

   database& db = this->db();

   uint32_t target_pow = db.get_pow_summary_target();
   FC_ASSERT( work.input.prev_block == db.head_block_id(), "pow not for last block" );
   FC_ASSERT( work.pow_summary < target_pow, "insufficient work difficulty" );

   FC_ASSERT( o.props.maximum_block_size >= STEEMIT_MIN_BLOCK_SIZE_LIMIT * 2, "maximum block size is too small" );

   const auto& dgp = db.get_dynamic_global_properties();
   db.modify( dgp, [&]( dynamic_global_property_object& p )
   {
      p.total_pow++;
      p.num_pow_witnesses++;
   });

   const auto& accounts_by_name = db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find( work.input.worker_account );
   if(itr == accounts_by_name.end())
   {
      FC_ASSERT( o.new_owner_key.valid(), "new owner key is not valid" );
      db.create< account_object >( [&]( account_object& acc )
      {
         acc.name = work.input.worker_account;
         acc.owner = authority( 1, *o.new_owner_key, 1);
         acc.active = acc.owner;
         acc.posting = acc.owner;
         acc.memo_key = *o.new_owner_key;
         acc.created = dgp.time;
         acc.last_vote_time = dgp.time;
         acc.recovery_account = ""; /// highest voted witness at time of recovery
      });
      db.create<witness_object>( [&]( witness_object& w )
      {
          w.owner             = work.input.worker_account;
          w.props             = o.props;
          w.signing_key       = *o.new_owner_key;
          w.pow_worker        = dgp.total_pow;
      });
   }
   else
   {
      FC_ASSERT( !o.new_owner_key.valid(), "cannot specify an owner key unless creating account" );
      const witness_object* cur_witness = db.find_witness( work.input.worker_account );
      FC_ASSERT( cur_witness, "Witness must be created for existing account before mining" );
      FC_ASSERT( cur_witness->pow_worker == 0, "this account is already scheduled for pow block production" );
      db.modify(*cur_witness, [&]( witness_object& w )
      {
          w.props             = o.props;
          w.pow_worker        = dgp.total_pow;
      });
   }

   /// pay the witness that includes this POW
   asset inc_reward = db.get_pow_reward();
   db.adjust_supply( inc_reward, true );

   const auto& inc_witness = db.get_account( dgp.current_witness );
   db.create_vesting( inc_witness, inc_reward );
}

void feed_publish_evaluator::do_apply( const feed_publish_operation& o )
{
  const auto& witness = db().get_witness( o.publisher );
  db().modify( witness, [&]( witness_object& w ){
      w.sbd_exchange_rate = o.exchange_rate;
      w.last_sbd_exchange_update = db().head_block_time();
  });
}

void convert_evaluator::do_apply( const convert_operation& o )
{
  const auto& owner = db().get_account( o.owner );
  FC_ASSERT( db().get_balance( owner, o.amount.symbol ) >= o.amount, "account does not sufficient balance" );

  db().adjust_balance( owner, -o.amount );

  const auto& fhistory = db().get_feed_history();
  FC_ASSERT( !fhistory.current_median_history.is_null(), "Cannot convert SBD because there is no price feed" );

  db().create<convert_request_object>( [&]( convert_request_object& obj )
  {
      obj.owner           = o.owner;
      obj.requestid       = o.requestid;
      obj.amount          = o.amount;
      obj.conversion_date = db().head_block_time() + STEEMIT_CONVERSION_DELAY; // 1 week
  });

}

void limit_order_create_evaluator::do_apply( const limit_order_create_operation& o )
{
   FC_ASSERT( o.expiration > db().head_block_time(), "limit order has to expire after head block time" );

   const auto& owner = db().get_account( o.owner );

   FC_ASSERT( db().get_balance( owner, o.amount_to_sell.symbol ) >= o.amount_to_sell, "account does not have sufficient funds for limit order" );

   db().adjust_balance( owner, -o.amount_to_sell );

   const auto& order = db().create<limit_order_object>( [&]( limit_order_object& obj )
   {
       obj.created    = db().head_block_time();
       obj.seller     = o.owner;
       obj.orderid    = o.orderid;
       obj.for_sale   = o.amount_to_sell.amount;
       obj.sell_price = o.get_price();
       obj.expiration = o.expiration;
   });

   bool filled = db().apply_order( order );

   if( o.fill_or_kill ) FC_ASSERT( filled, "cancelling order because it was not filled" );
}

void limit_order_create2_evaluator::do_apply( const limit_order_create2_operation& o )
{
   FC_ASSERT( o.expiration > db().head_block_time(), "limit order has to expire after head block time" );

   const auto& owner = db().get_account( o.owner );

   FC_ASSERT( db().get_balance( owner, o.amount_to_sell.symbol ) >= o.amount_to_sell, "account does not have sufficient funds for limit order" );

   db().adjust_balance( owner, -o.amount_to_sell );

   const auto& order = db().create<limit_order_object>( [&]( limit_order_object& obj )
   {
       obj.created    = db().head_block_time();
       obj.seller     = o.owner;
       obj.orderid    = o.orderid;
       obj.for_sale   = o.amount_to_sell.amount;
       obj.sell_price = o.exchange_rate;
       obj.expiration = o.expiration;
   });

   bool filled = db().apply_order( order );

   if( o.fill_or_kill ) FC_ASSERT( filled, "cancelling order because it was not filled" );
}

void limit_order_cancel_evaluator::do_apply( const limit_order_cancel_operation& o )
{
   db().cancel_order( db().get_limit_order( o.owner, o.orderid ) );
}

void report_over_production_evaluator::do_apply( const report_over_production_operation& o )
{
   FC_ASSERT( !db().has_hardfork( STEEMIT_HARDFORK_0_4 ), "this operation is disabled" );
}

void challenge_authority_evaluator::do_apply( const challenge_authority_operation& o )
{
   const auto& challenged = db().get_account( o.challenged );
   const auto& challenger = db().get_account( o.challenger );

   if( o.require_owner )
   {
      FC_ASSERT( false, "Challenging the owner key is not supported at this time" );
#if 0
      FC_ASSERT( challenger.balance >= STEEMIT_OWNER_CHALLENGE_FEE );
      FC_ASSERT( !challenged.owner_challenged );
      FC_ASSERT( db().head_block_time() - challenged.last_owner_proved > STEEMIT_OWNER_CHALLENGE_COOLDOWN );

      db().adjust_balance( challenger, - STEEMIT_OWNER_CHALLENGE_FEE );
      db().create_vesting( db().get_account( o.challenged ), STEEMIT_OWNER_CHALLENGE_FEE );

      db().modify( challenged, [&]( account_object& a )
      {
         a.owner_challenged = true;
      });
#endif
  }
  else
  {
      FC_ASSERT( challenger.balance >= STEEMIT_ACTIVE_CHALLENGE_FEE, "account does not have sufficient funds to pay challenge fee" );
      FC_ASSERT( !( challenged.owner_challenged || challenged.active_challenged ), "account is already challenged" );
      FC_ASSERT( db().head_block_time() - challenged.last_active_proved < STEEMIT_ACTIVE_CHALLENGE_COOLDOWN, "account cannot be challenged because it was recently challenged" );
      if( !db().has_hardfork( STEEMIT_HARDFORK_0_14__307 ) ) // This assert should have been removed during hf11 TODO: Remove after HF 14
         FC_ASSERT( db().head_block_time() - challenged.last_active_proved > STEEMIT_ACTIVE_CHALLENGE_COOLDOWN );

      db().adjust_balance( challenger, - STEEMIT_ACTIVE_CHALLENGE_FEE );
      db().create_vesting( db().get_account( o.challenged ), STEEMIT_ACTIVE_CHALLENGE_FEE );

      db().modify( challenged, [&]( account_object& a )
      {
         a.active_challenged = true;
      });
  }
}

void prove_authority_evaluator::do_apply( const prove_authority_operation& o )
{
   const auto& challenged = db().get_account( o.challenged );
   FC_ASSERT( challenged.owner_challenged || challenged.active_challenged, "Account is not challeneged. No need to prove authority." );

   db().modify( challenged, [&]( account_object& a )
   {
      a.active_challenged = false;
      a.last_active_proved = db().head_block_time();
      if( o.require_owner )
      {
         a.owner_challenged = false;
         a.last_owner_proved = db().head_block_time();
      }
   });
}

void request_account_recovery_evaluator::do_apply( const request_account_recovery_operation& o )
{
   const auto& account_to_recover = db().get_account( o.account_to_recover );

   if ( account_to_recover.recovery_account.length() )   // Make sure recovery matches expected recovery account
      FC_ASSERT( account_to_recover.recovery_account == o.recovery_account, "cannot recover an account that does not have you as there recovery partner" );
   else                                                  // Empty string recovery account defaults to top witness
      FC_ASSERT( db().get_index_type< witness_index >().indices().get< by_vote_name >().begin()->owner == o.recovery_account, "top witness must recover an account with no recovery partner" );

   const auto& recovery_request_idx = db().get_index_type< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   if( request == recovery_request_idx.end() ) // New Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover with an impossible authority" );
      FC_ASSERT( o.new_owner_authority.weight_threshold, "Cannot recover with an open authority" );

      // Check accounts in the new authority exist
      db().create< account_recovery_request_object >( [&]( account_recovery_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.new_owner_authority = o.new_owner_authority;
         req.expires = db().head_block_time() + STEEMIT_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
   else if( o.new_owner_authority.weight_threshold == 0 ) // Cancel Request if authority is open
   {
      db().remove( *request );
   }
   else // Change Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover with an impossible authority" );

      db().modify( *request, [&]( account_recovery_request_object& req )
      {
         req.new_owner_authority = o.new_owner_authority;
         req.expires = db().head_block_time() + STEEMIT_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
}

void recover_account_evaluator::do_apply( const recover_account_operation& o )
{
   const auto& account = db().get_account( o.account_to_recover );

   if( db().has_hardfork( STEEMIT_HARDFORK_0_12 ) )
      FC_ASSERT( db().head_block_time() - account.last_account_recovery > STEEMIT_OWNER_UPDATE_LIMIT, "owner authority can only be updated once an hour" );

   const auto& recovery_request_idx = db().get_index_type< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   FC_ASSERT( request != recovery_request_idx.end(), "there are no active recovery requests for this account" );
   FC_ASSERT( request->new_owner_authority == o.new_owner_authority, "new owner authority does not match recovery request" );

   const auto& recent_auth_idx = db().get_index_type< owner_authority_history_index >().indices().get< by_account >();
   auto hist = recent_auth_idx.lower_bound( o.account_to_recover );
   bool found = false;

   while( hist != recent_auth_idx.end() && hist->account == o.account_to_recover && !found )
   {
      found = hist->previous_owner_authority == o.recent_owner_authority;
      if( found ) break;
      ++hist;
   }

   FC_ASSERT( found, "Recent authority not found in authority history" );

   db().remove( *request ); // Remove first, update_owner_authority may invalidate iterator
   db().update_owner_authority( account, o.new_owner_authority );
   db().modify( account, [&]( account_object& a )
   {
      a.last_account_recovery = db().head_block_time();
   });
}

void change_recovery_account_evaluator::do_apply( const change_recovery_account_operation& o )
{
   db().get_account( o.new_recovery_account ); // Simply validate account exists
   const auto& account_to_recover = db().get_account( o.account_to_recover );

   const auto& change_recovery_idx = db().get_index_type< change_recovery_account_request_index >().indices().get< by_account >();
   auto request = change_recovery_idx.find( o.account_to_recover );

   if( request == change_recovery_idx.end() ) // New request
   {
      db().create< change_recovery_account_request_object >( [&]( change_recovery_account_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.recovery_account = o.new_recovery_account;
         req.effective_on = db().head_block_time() + STEEMIT_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else if( account_to_recover.recovery_account != o.new_recovery_account ) // Change existing request
   {
      db().modify( *request, [&]( change_recovery_account_request_object& req )
      {
         req.recovery_account = o.new_recovery_account;
         req.effective_on = db().head_block_time() + STEEMIT_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else // Request exists and changing back to current recovery account
   {
      db().remove( *request );
   }
}

} } // steemit::chain
