#include <steem/plugins/tags_api/tags_api_plugin.hpp>
#include <steem/plugins/tags_api/tags_api.hpp>
#include <steem/plugins/tags/tags_plugin.hpp>
#include <steem/plugins/follow_api/follow_api_plugin.hpp>
#include <steem/plugins/follow_api/follow_api.hpp>

#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/util/reward.hpp>
#include <steem/chain/util/uint256.hpp>

namespace steem { namespace plugins { namespace tags {

namespace detail {

class tags_api_impl
{
   public:
      tags_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

      DECLARE_API(
         (get_trending_tags)
         (get_tags_used_by_author)
         (get_discussion)
         (get_content_replies)
         (get_post_discussions_by_payout)
         (get_comment_discussions_by_payout)
         (get_discussions_by_trending)
         (get_discussions_by_created)
         (get_discussions_by_active)
         (get_discussions_by_cashout)
         (get_discussions_by_votes)
         (get_discussions_by_children)
         (get_discussions_by_hot)
         (get_discussions_by_feed)
         (get_discussions_by_blog)
         (get_discussions_by_comments)
         (get_discussions_by_promoted)
         (get_replies_by_last_update)
         (get_discussions_by_author_before_date)
         (get_active_votes)
      )

      void set_pending_payout( discussion& d );
      void set_url( discussion& d );
      discussion lookup_discussion( chain::comment_id_type, uint32_t truncate_body = 0 );

      static bool filter_default( const database_api::api_comment_object& c ) { return false; }
      static bool exit_default( const database_api::api_comment_object& c )   { return false; }
      static bool tag_exit_default( const tags::tag_object& c )               { return false; }

      template<typename Index, typename StartItr>
      discussion_query_result get_discussions( const discussion_query& q,
                                               const string& tag,
                                               chain::comment_id_type parent,
                                               const Index& idx, StartItr itr,
                                               uint32_t truncate_body = 0,
                                               const std::function< bool( const database_api::api_comment_object& ) >& filter = &tags_api_impl::filter_default,
                                               const std::function< bool( const database_api::api_comment_object& ) >& exit   = &tags_api_impl::exit_default,
                                               const std::function< bool( const tags::tag_object& ) >& tag_exit               = &tags_api_impl::tag_exit_default,
                                               bool ignore_parent = false
                                               );

      chain::comment_id_type get_parent( const discussion_query& q );

      chain::database& _db;
      std::shared_ptr< steem::plugins::follow::follow_api > _follow_api;
};

DEFINE_API( tags_api_impl, get_trending_tags )
{
   FC_ASSERT( args.limit <= 1000, "Cannot retrieve more than 1000 tags at a time." );
   get_trending_tags_return result;
   result.tags.reserve( args.limit );

   const auto& nidx = _db.get_index< tags::tag_stats_index, tags::by_tag >();

   const auto& ridx = _db.get_index< tags::tag_stats_index, tags::by_trending >();
   auto itr = ridx.begin();
   if( args.start_tag != "" && nidx.size() )
   {
      auto nitr = nidx.lower_bound( args.start_tag );
      if( nitr == nidx.end() )
         itr = ridx.end();
      else
         itr = ridx.iterator_to( *nitr );
   }

   while( itr != ridx.end() && result.tags.size() < args.limit )
   {
      result.tags.push_back( api_tag_object( *itr ) );
      ++itr;
   }
   return result;
}

DEFINE_API( tags_api_impl, get_tags_used_by_author )
{
   const auto* acnt = _db.find_account( args.author );
   FC_ASSERT( acnt != nullptr, "Author not found" );

   const auto& tidx = _db.get_index< tags::author_tag_stats_index, tags::by_author_posts_tag >();
   auto itr = tidx.lower_bound( boost::make_tuple( acnt->id, 0 ) );

   get_tags_used_by_author_return result;

   while( itr != tidx.end() && itr->author == acnt->id && result.tags.size() < 1000 )
   {
      result.tags.push_back( tag_count_object( { itr->tag, itr->total_posts } ) );
      ++itr;
   }

   return result;
}

DEFINE_API( tags_api_impl, get_discussion )
{
   const auto& by_permlink_idx = _db.get_index< chain::comment_index, chain::by_permlink >();
   auto itr = by_permlink_idx.find( boost::make_tuple( args.author, args.permlink ) );

   if( itr != by_permlink_idx.end() )
   {
      get_discussion_return result( *itr, _db );
      set_pending_payout( result );
      result.active_votes = get_active_votes( get_active_votes_args( { args.author, args.permlink } ) ).votes;
      return result;
   }

   return get_discussion_return();
}

DEFINE_API( tags_api_impl, get_content_replies )
{
   const auto& by_permlink_idx = _db.get_index< chain::comment_index, chain::by_parent >();
   auto itr = by_permlink_idx.find( boost::make_tuple( args.author, args.permlink ) );

   get_content_replies_return result;

   while( itr != by_permlink_idx.end() && itr->parent_author == args.author && chain::to_string( itr->parent_permlink ) == args.permlink )
   {
      result.discussions.push_back( discussion( *itr, _db ) );
      set_pending_payout( result.discussions.back() );
      ++itr;
   }
   return result;
}

DEFINE_API( tags_api_impl, get_post_discussions_by_payout )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = chain::comment_id_type();

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_reward_fund_net_rshares >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, true ) );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body, []( const database_api::api_comment_object& c ){ return c.net_rshares <= 0; }, exit_default, tag_exit_default, true );
}

DEFINE_API( tags_api_impl, get_comment_discussions_by_payout )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = chain::comment_id_type( 1 );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_reward_fund_net_rshares >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, false ) );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body, []( const database_api::api_comment_object& c ){ return c.net_rshares <= 0; }, exit_default, tag_exit_default, true );
}

DEFINE_API( tags_api_impl, get_discussions_by_trending )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_trending >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, std::numeric_limits< double >::max() )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body, []( const database_api::api_comment_object& c ) { return c.net_rshares <= 0; } );
}

DEFINE_API( tags_api_impl, get_discussions_by_created )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_created >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, fc::time_point_sec::maximum() )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body );
}

DEFINE_API( tags_api_impl, get_discussions_by_active )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_active >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, fc::time_point_sec::maximum() )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body );
}

DEFINE_API( tags_api_impl, get_discussions_by_cashout )
{
   args.validate();
   vector<discussion> result;

   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_cashout >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, fc::time_point::now() - fc::minutes( 60 ) ) );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body, []( const database_api::api_comment_object& c ){ return c.net_rshares < 0; });
}

DEFINE_API( tags_api_impl, get_discussions_by_votes )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_net_votes >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, std::numeric_limits< int32_t >::max() )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body );
}

DEFINE_API( tags_api_impl, get_discussions_by_children )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_children >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, std::numeric_limits< int32_t >::max() )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body );
}

DEFINE_API( tags_api_impl, get_discussions_by_hot )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_hot >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, std::numeric_limits< double >::max() )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body, []( const database_api::api_comment_object& c ) { return c.net_rshares <= 0; } );
}

DEFINE_API( tags_api_impl, get_discussions_by_feed )
{
   args.validate();
   FC_ASSERT( _db.has_index< follow::feed_index >(), "Node is not running the follow plugin" );
   auto start_author = args.start_author ? *( args.start_author ) : "";
   auto start_permlink = args.start_permlink ? *( args.start_permlink ) : "";

   const auto& account = _db.get_account( args.tag );

   const auto& c_idx = _db.get_index< follow::feed_index, follow::by_comment >();
   const auto& f_idx = _db.get_index< follow::feed_index, follow::by_feed >();
   auto feed_itr = f_idx.lower_bound( account.name );

   if( start_author.size() || start_permlink.size() )
   {
      auto start_c = c_idx.find( boost::make_tuple( _db.get_comment( start_author, start_permlink ).id, account.name/*, 0 blocked*/ ) );
      FC_ASSERT( start_c != c_idx.end(), "Comment is not in account's feed" );
      feed_itr = f_idx.iterator_to( *start_c );
   }

   get_discussions_by_feed_return result;
   result.discussions.reserve( args.limit );

   while( result.discussions.size() < args.limit /*&& feed_itr->blocked == 0*/ && feed_itr != f_idx.end() )
   {
      if( feed_itr->account != account.name )
         break;
      try
      {
         result.discussions.push_back( lookup_discussion( feed_itr->comment ) );
         if( feed_itr->first_reblogged_by != account_name_type() )
         {
            result.discussions.back().reblogged_by = vector<account_name_type>( feed_itr->reblogged_by.begin(), feed_itr->reblogged_by.end() );
            result.discussions.back().first_reblogged_by = feed_itr->first_reblogged_by;
            result.discussions.back().first_reblogged_on = feed_itr->first_reblogged_on;
         }
      }
      catch ( const fc::exception& e )
      {
         edump((e.to_detail_string()));
      }

      ++feed_itr;
   }
   return result;
}

DEFINE_API( tags_api_impl, get_discussions_by_blog )
{
   args.validate();
   FC_ASSERT( _db.has_index< follow::blog_index >(), "Node is not running the follow plugin" );
   auto start_author = args.start_author ? *( args.start_author ) : "";
   auto start_permlink = args.start_permlink ? *( args.start_permlink ) : "";

   const auto& account = _db.get_account( args.tag );

   const auto& tag_idx = _db.get_index<tags::tag_index>().indices().get<tags::by_comment>();

   const auto& c_idx = _db.get_index< follow::blog_index, follow::by_comment >();
   const auto& b_idx = _db.get_index< follow::blog_index, follow::by_blog >();
   auto blog_itr = b_idx.lower_bound( account.name );

   if( start_author.size() || start_permlink.size() )
   {
      auto start_c = c_idx.find( boost::make_tuple( _db.get_comment( start_author, start_permlink ).id, account.name ) );
      FC_ASSERT( start_c != c_idx.end(), "Comment is not in account's blog" );
      blog_itr = b_idx.iterator_to( *start_c );
   }

   get_discussions_by_blog_return result;
   result.discussions.reserve( args.limit );

   while( result.discussions.size() < args.limit && blog_itr != b_idx.end() )
   {
      if( blog_itr->account != account.name )
         break;
      try
      {
         if( args.select_authors.size() &&
               args.select_authors.find( blog_itr->account ) == args.select_authors.end() ) {
            ++blog_itr;
            continue;
         }

         if( args.select_tags.size() ) {
            auto tag_itr = tag_idx.lower_bound( blog_itr->comment );

            bool found = false;
            while( tag_itr != tag_idx.end() && tag_itr->comment == blog_itr->comment ) {
               if( args.select_tags.find( tag_itr->tag ) != args.select_tags.end() ) {
                  found = true; break;
               }
               ++tag_itr;
            }
            if( !found ) {
               ++blog_itr;
               continue;
            }
         }

         result.discussions.push_back( lookup_discussion( blog_itr->comment, args.truncate_body ) );
         if( blog_itr->reblogged_on > time_point_sec() )
         {
            result.discussions.back().first_reblogged_on = blog_itr->reblogged_on;
         }
      }
      catch ( const fc::exception& e )
      {
         edump((e.to_detail_string()));
      }

      ++blog_itr;
   }
   return result;
}

DEFINE_API( tags_api_impl, get_discussions_by_comments )
{
   get_discussions_by_comments_return result;
#ifndef IS_LOW_MEM
   args.validate();
   FC_ASSERT( args.start_author, "Must get comments for a specific author" );
   auto start_author = *( args.start_author );
   auto start_permlink = args.start_permlink ? *( args.start_permlink ) : "";

   const auto& c_idx = _db.get_index< comment_index, by_permlink >();
   const auto& t_idx = _db.get_index< comment_index, by_author_last_update >();
   auto comment_itr = t_idx.lower_bound( start_author );

   if( start_permlink.size() )
   {
      auto start_c = c_idx.find( boost::make_tuple( start_author, start_permlink ) );
      FC_ASSERT( start_c != c_idx.end(), "Comment is not in account's comments" );
      comment_itr = t_idx.iterator_to( *start_c );
   }

   result.discussions.reserve( args.limit );

   while( result.discussions.size() < args.limit && comment_itr != t_idx.end() )
   {
      if( comment_itr->author != start_author )
         break;
      if( comment_itr->parent_author.size() > 0 )
      {
         try
         {
            result.discussions.push_back( lookup_discussion( comment_itr->id ) );
         }
         catch( const fc::exception& e )
         {
            edump( (e.to_detail_string() ) );
         }
      }

      ++comment_itr;
   }
#endif
   return result;
}

DEFINE_API( tags_api_impl, get_discussions_by_promoted )
{
   args.validate();
   auto tag = fc::to_lower( args.tag );
   auto parent = get_parent( args );

   const auto& tidx = _db.get_index< tags::tag_index, tags::by_parent_promoted >();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, share_type( STEEM_MAX_SHARE_SUPPLY ) )  );

   return get_discussions( args, tag, parent, tidx, tidx_itr, args.truncate_body, filter_default, exit_default, []( const tags::tag_object& t ){ return t.promoted_balance == 0; }  );
}

DEFINE_API( tags_api_impl, get_replies_by_last_update )
{
   get_replies_by_last_update_return result;

#ifndef IS_LOW_MEM
   FC_ASSERT( args.limit <= 100 );
   const auto& last_update_idx = _db.get_index< comment_index, by_last_update >();
   auto itr = last_update_idx.begin();
   account_name_type parent_author = args.start_parent_author;

   if( args.start_permlink.size() )
   {
      const auto& comment = _db.get_comment( args.start_parent_author, args.start_permlink );
      itr = last_update_idx.iterator_to( comment );
      parent_author = comment.parent_author;
   }
   else if( args.start_parent_author.size() )
   {
      itr = last_update_idx.lower_bound( args.start_parent_author );
   }

   result.discussions.reserve( args.limit );

   while( itr != last_update_idx.end() && result.discussions.size() < args.limit && itr->parent_author == parent_author )
   {
      result.discussions.push_back( discussion( *itr, _db ) );
      set_pending_payout( result.discussions.back() );
      result.discussions.back().active_votes = get_active_votes( get_active_votes_args( { itr->author, chain::to_string( itr->permlink ) } ) ).votes;
      ++itr;
   }

#endif
   return result;
}

DEFINE_API( tags_api_impl, get_discussions_by_author_before_date )
{
   try
   {
      get_discussions_by_author_before_date_return result;
#ifndef IS_LOW_MEM
      FC_ASSERT( args.limit <= 100 );
      result.discussions.reserve( args.limit );
      uint32_t count = 0;
      const auto& didx = _db.get_index< comment_index, by_author_last_update >();

      auto before_date = args.before_date;

      if( before_date == time_point_sec() )
         before_date = time_point_sec::maximum();

      auto itr = didx.lower_bound( boost::make_tuple( args.author, time_point_sec::maximum() ) );
      if( args.start_permlink.size() )
      {
         const auto& comment = _db.get_comment( args.author, args.start_permlink );
         if( comment.created < before_date )
            itr = didx.iterator_to( comment );
      }


      while( itr != didx.end() && itr->author == args.author && count < args.limit )
      {
         if( itr->parent_author.size() == 0 )
         {
            result.discussions.push_back( discussion( *itr, _db ) );
            set_pending_payout( result.discussions.back() );
            result.discussions.back().active_votes = get_active_votes( get_active_votes_args( { itr->author, chain::to_string( itr->permlink ) } ) ).votes;
            ++count;
         }
         ++itr;
      }

#endif
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (args) )
}

DEFINE_API( tags_api_impl, get_active_votes )
{
   get_active_votes_return result;
   const auto& comment = _db.get_comment( args.author, args.permlink );
   const auto& idx = _db.get_index< chain::comment_vote_index, chain::by_comment_voter >();
   chain::comment_id_type cid(comment.id);
   auto itr = idx.lower_bound( cid );
   while( itr != idx.end() && itr->comment == cid )
   {
      const auto& vo = _db.get( itr->voter );
      vote_state vstate;
      vstate.voter = vo.name;
      vstate.weight = itr->weight;
      vstate.rshares = itr->rshares;
      vstate.percent = itr->vote_percent;
      vstate.time = itr->last_update;

      if( _follow_api )
      {
         auto reps = _follow_api->get_account_reputations( follow::get_account_reputations_args( { vo.name, 1 } ) ).reputations;
         if( reps.size() )
            vstate.reputation = reps[0].reputation;
      }

      result.votes.push_back( vstate );
      ++itr;
   }
   return result;
}

void tags_api_impl::set_pending_payout( discussion& d )
{
   const auto& cidx = _db.get_index< tags::tag_index, tags::by_comment>();
   auto itr = cidx.lower_bound( d.id );
   if( itr != cidx.end() && itr->comment == d.id )  {
      d.promoted = asset( itr->promoted_balance, SBD_SYMBOL );
   }

   const auto& props = _db.get_dynamic_global_properties();
   const auto& hist  = _db.get_feed_history();

   asset pot;
   if( _db.has_hardfork( STEEM_HARDFORK_0_17__774 ) )
      pot = _db.get_reward_fund( _db.get_comment( d.author, d.permlink ) ).reward_balance;
   else
      pot = props.total_reward_fund_steem;

   if( !hist.current_median_history.is_null() ) pot = pot * hist.current_median_history;

   u256 total_r2 = 0;
   if( _db.has_hardfork( STEEM_HARDFORK_0_17__774 ) )
      total_r2 = chain::util::to256( _db.get_reward_fund( _db.get_comment( d.author, d.permlink ) ).recent_claims );
   else
      total_r2 = chain::util::to256( props.total_reward_shares2 );

   if( total_r2 > 0 )
   {
      uint128_t vshares;
      if( _db.has_hardfork( STEEM_HARDFORK_0_17__774 ) )
      {
         const auto& rf = _db.get_reward_fund( _db.get_comment( d.author, d.permlink ) );
         vshares = d.net_rshares.value > 0 ? chain::util::evaluate_reward_curve( d.net_rshares.value, rf.author_reward_curve, rf.content_constant ) : 0;
      }
      else
         vshares = d.net_rshares.value > 0 ? chain::util::evaluate_reward_curve( d.net_rshares.value ) : 0;

      u256 r2 = chain::util::to256( vshares ); //to256(abs_net_rshares);
      r2 *= pot.amount.value;
      r2 /= total_r2;

      d.pending_payout_value = asset( static_cast<uint64_t>(r2), pot.symbol );

      if( _follow_api )
      {
         d.author_reputation = _follow_api->get_account_reputations( follow::get_account_reputations_args( { d.author, 1} ) ).reputations[0].reputation;
      }
   }

   if( d.parent_author != STEEM_ROOT_POST_PARENT )
      d.cashout_time = _db.calculate_discussion_payout_time( _db.get< chain::comment_object >( d.id ) );

   if( d.body.size() > 1024*128 )
      d.body = "body pruned due to size";
   if( d.parent_author.size() > 0 && d.body.size() > 1024*16 )
      d.body = "comment pruned due to size";

   set_url( d );
}

void tags_api_impl::set_url( discussion& d )
{
   const database_api::api_comment_object root( _db.get_comment( d.root_author, d.root_permlink ), _db );
   d.url = "/" + root.category + "/@" + root.author + "/" + root.permlink;
   d.root_title = root.title;
   if( root.id != d.id )
      d.url += "#@" + d.author + "/" + d.permlink;
}

discussion tags_api_impl::lookup_discussion( chain::comment_id_type id, uint32_t truncate_body )
{
   discussion d( _db.get( id ), _db );
   set_url( d );
   set_pending_payout( d );
   d.active_votes = get_active_votes( get_active_votes_args( { d.author, d.permlink } ) ).votes;
   d.body_length = d.body.size();
   if( truncate_body )
   {
      d.body = d.body.substr( 0, truncate_body );

      if( !fc::is_utf8( d.body ) )
         d.body = fc::prune_invalid_utf8( d.body );
   }
   return d;
}

template<typename Index, typename StartItr>
discussion_query_result tags_api_impl::get_discussions( const discussion_query& query,
                                                        const string& tag,
                                                        chain::comment_id_type parent,
                                                        const Index& tidx, StartItr tidx_itr,
                                                        uint32_t truncate_body,
                                                        const std::function< bool( const database_api::api_comment_object& ) >& filter,
                                                        const std::function< bool( const database_api::api_comment_object& ) >& exit,
                                                        const std::function< bool( const tags::tag_object& ) >& tag_exit,
                                                        bool ignore_parent
                                                        )
{
   discussion_query_result result;

   const auto& cidx = _db.get_index< tags::tag_index, tags::by_comment >();
   chain::comment_id_type start;

   if( query.start_author && query.start_permlink )
   {
      start = _db.get_comment( *query.start_author, *query.start_permlink ).id;
      auto itr = cidx.find( start );
      while( itr != cidx.end() && itr->comment == start )
      {
         if( itr->tag == tag )
         {
            tidx_itr = tidx.iterator_to( *itr );
            break;
         }
         ++itr;
      }
   }

   uint32_t count = query.limit;
   uint64_t itr_count = 0;
   uint64_t filter_count = 0;
   uint64_t exc_count = 0;
   uint64_t max_itr_count = 10 * query.limit;
   while( count > 0 && tidx_itr != tidx.end() )
   {
      ++itr_count;
      if( itr_count > max_itr_count )
      {
         wlog( "Maximum iteration count exceeded serving query: ${q}", ("q", query) );
         wlog( "count=${count}   itr_count=${itr_count}   filter_count=${filter_count}   exc_count=${exc_count}",
               ("count", count)("itr_count", itr_count)("filter_count", filter_count)("exc_count", exc_count) );
         break;
      }
      if( tidx_itr->tag != tag || ( !ignore_parent && tidx_itr->parent != parent ) )
         break;
      try
      {
         result.discussions.push_back( lookup_discussion( tidx_itr->comment, truncate_body ) );
         result.discussions.back().promoted = asset(tidx_itr->promoted_balance, SBD_SYMBOL );

         if( filter( result.discussions.back() ) )
         {
            result.discussions.pop_back();
            ++filter_count;
         }
         else if( exit( result.discussions.back() ) || tag_exit( *tidx_itr )  )
         {
            result.discussions.pop_back();
            break;
         }
         else
            --count;
      }
      catch ( const fc::exception& e )
      {
         ++exc_count;
         edump((e.to_detail_string()));
      }
      ++tidx_itr;
   }
   return result;
}

chain::comment_id_type tags_api_impl::get_parent( const discussion_query& query )
{
   chain::comment_id_type parent;
   if( query.parent_author && query.parent_permlink ) {
      parent = _db.get_comment( *query.parent_author, *query.parent_permlink ).id;
   }
   return parent;
}

} // detail

tags_api::tags_api(): my( new detail::tags_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_TAGS_API_PLUGIN_NAME );
}

tags_api::~tags_api() {}

DEFINE_API( tags_api, get_trending_tags )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_trending_tags( args );
   });
}

DEFINE_API( tags_api, get_tags_used_by_author )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_tags_used_by_author( args );
   });
}

DEFINE_API( tags_api, get_discussion )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussion( args );
   });
}

DEFINE_API( tags_api, get_content_replies )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_content_replies( args );
   });
}

DEFINE_API( tags_api, get_post_discussions_by_payout )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_post_discussions_by_payout( args );
   });
}

DEFINE_API( tags_api, get_comment_discussions_by_payout )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_comment_discussions_by_payout( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_trending )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_trending( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_created )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_created( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_active )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_active( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_cashout )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_cashout( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_votes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_votes( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_children )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_children( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_hot )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_hot( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_feed )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_feed( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_blog )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_blog( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_comments )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_comments( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_promoted )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_promoted( args );
   });
}

DEFINE_API( tags_api, get_replies_by_last_update )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_replies_by_last_update( args );
   });
}

DEFINE_API( tags_api, get_discussions_by_author_before_date )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_discussions_by_author_before_date( args );
   });
}

DEFINE_API( tags_api, get_active_votes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_active_votes( args );
   });
}

void tags_api::set_pending_payout( discussion& d )
{
   my->set_pending_payout( d );
}

void tags_api::api_startup()
{
   auto follow_api_plugin = appbase::app().find_plugin< steem::plugins::follow::follow_api_plugin >();

   if( follow_api_plugin != nullptr )
      my->_follow_api = follow_api_plugin->api;
}

} } } // steem::plugins::tags
