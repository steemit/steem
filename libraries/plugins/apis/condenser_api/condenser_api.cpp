#include <steem/plugins/condenser_api/condenser_api.hpp>
#include <steem/plugins/condenser_api/condenser_api_plugin.hpp>

#include <steem/plugins/database_api/database_api_plugin.hpp>
#include <steem/plugins/block_api/block_api_plugin.hpp>
#include <steem/plugins/account_history_api/account_history_api_plugin.hpp>
#include <steem/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <steem/plugins/tags_api/tags_api_plugin.hpp>
#include <steem/plugins/follow_api/follow_api_plugin.hpp>
#include <steem/plugins/reputation_api/reputation_api_plugin.hpp>
#include <steem/plugins/market_history_api/market_history_api_plugin.hpp>

#include <steem/utilities/git_revision.hpp>

#include <steem/chain/util/reward.hpp>
#include <steem/chain/util/uint256.hpp>

#include <fc/git_revision.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/lock_guard.hpp>

#define CHECK_ARG_SIZE( s ) \
   FC_ASSERT( args.size() == s, "Expected #s argument(s), was ${n}", ("n", args.size()) );

namespace steem { namespace plugins { namespace condenser_api {

namespace detail
{
   typedef std::function< void( const broadcast_transaction_synchronous_return& ) > confirmation_callback;

   class condenser_api_impl
   {
      public:
         condenser_api_impl() :
            _chain( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >() ),
            _db( _chain.db() )
         {
            _on_post_apply_block_conn = _db.add_post_apply_block_handler(
               [&]( const block_notification& note ){ on_post_apply_block( note.block ); },
               appbase::app().get_plugin< steem::plugins::condenser_api::condenser_api_plugin >(),
               0 );
         }

         DECLARE_API_IMPL(
            (get_version)
            (get_trending_tags)
            (get_state)
            (get_active_witnesses)
            (get_block_header)
            (get_block)
            (get_ops_in_block)
            (get_config)
            (get_dynamic_global_properties)
            (get_chain_properties)
            (get_current_median_history_price)
            (get_feed_history)
            (get_witness_schedule)
            (get_hardfork_version)
            (get_next_scheduled_hardfork)
            (get_reward_fund)
            (get_key_references)
            (get_accounts)
            (get_account_references)
            (lookup_account_names)
            (lookup_accounts)
            (get_account_count)
            (get_owner_history)
            (get_recovery_request)
            (get_escrow)
            (get_withdraw_routes)
            (get_savings_withdraw_from)
            (get_savings_withdraw_to)
            (get_vesting_delegations)
            (get_expiring_vesting_delegations)
            (get_witnesses)
            (get_conversion_requests)
            (get_witness_by_account)
            (get_witnesses_by_vote)
            (lookup_witness_accounts)
            (get_witness_count)
            (get_open_orders)
            (get_transaction_hex)
            (get_transaction)
            (get_required_signatures)
            (get_potential_signatures)
            (verify_authority)
            (verify_account_authority)
            (get_active_votes)
            (get_account_votes)
            (get_content)
            (get_content_replies)
            (get_tags_used_by_author)
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
            (get_account_history)
            (broadcast_transaction)
            (broadcast_transaction_synchronous)
            (broadcast_block)
            (get_followers)
            (get_following)
            (get_follow_count)
            (get_feed_entries)
            (get_feed)
            (get_blog_entries)
            (get_blog)
            (get_account_reputations)
            (get_reblogged_by)
            (get_blog_authors)
            (get_ticker)
            (get_volume)
            (get_order_book)
            (get_trade_history)
            (get_recent_trades)
            (get_market_history)
            (get_market_history_buckets)
         )

         void recursively_fetch_content( state& _state, tags::discussion& root, set<string>& referenced_accounts );

         void set_pending_payout( discussion& d );

         void on_post_apply_block( const signed_block& b );

         steem::plugins::chain::chain_plugin&                              _chain;

         chain::database&                                                  _db;

         std::shared_ptr< database_api::database_api >                     _database_api;
         std::shared_ptr< block_api::block_api >                           _block_api;
         std::shared_ptr< account_history::account_history_api >           _account_history_api;
         std::shared_ptr< account_by_key::account_by_key_api >             _account_by_key_api;
         std::shared_ptr< network_broadcast_api::network_broadcast_api >   _network_broadcast_api;
         p2p::p2p_plugin*                                                  _p2p = nullptr;
         std::shared_ptr< tags::tags_api >                                 _tags_api;
         std::shared_ptr< follow::follow_api >                             _follow_api;
         std::shared_ptr< reputation::reputation_api >                     _reputation_api;
         std::shared_ptr< market_history::market_history_api >             _market_history_api;

         map< transaction_id_type, confirmation_callback >                 _callbacks;
         map< time_point_sec, vector< transaction_id_type > >              _callback_expirations;
         boost::signals2::connection                                       _on_post_apply_block_conn;

         boost::mutex                                                      _mtx;
   };

   DEFINE_API_IMPL( condenser_api_impl, get_version )
   {
      CHECK_ARG_SIZE( 0 )
      return get_version_return
      (
         fc::string( STEEM_BLOCKCHAIN_VERSION ),
         fc::string( steem::utilities::git_revision_sha ),
         fc::string( fc::git_revision_sha )
      );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_trending_tags )
   {
      CHECK_ARG_SIZE( 2 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto tags = _tags_api->get_trending_tags( { args[0].as< string >(), args[1].as< uint32_t >() } ).tags;
      vector< api_tag_object > result;

      for( const auto& t : tags )
      {
         result.push_back( api_tag_object( t ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_state )
   {
      CHECK_ARG_SIZE( 1 )
      string path = args[0].as< string >();

      state _state;
      _state.props         = get_dynamic_global_properties( {} );
      _state.current_route = path;
      _state.feed_price    = _database_api->get_current_price_feed( {} );

      try
      {
         if( path.size() && path[0] == '/' )
            path = path.substr(1); /// remove '/' from front

         if( !path.size() )
            path = "trending";

         /// FETCH CATEGORY STATE
         if( _tags_api )
         {
            auto trending_tags = _tags_api->get_trending_tags( { std::string(), 50 } ).tags;
            for( const auto& t : trending_tags )
            {
               _state.tag_idx.trending.push_back( t.name );
            }
         }
         /// END FETCH CATEGORY STATE

         set<string> accounts;

         vector<string> part; part.reserve(4);
         boost::split( part, path, boost::is_any_of("/") );
         part.resize(std::max( part.size(), size_t(4) ) ); // at least 4

         auto tag = fc::to_lower( part[1] );

         if( part[0].size() && part[0][0] == '@' ) {
            auto acnt = part[0].substr(1);
            _state.accounts[acnt] = extended_account( database_api::api_account_object( _db.get_account( acnt ), _db ) );

            if( _tags_api )
               _state.accounts[acnt].tags_usage = _tags_api->get_tags_used_by_author( { acnt } ).tags;

            if( _follow_api )
            {
               _state.accounts[acnt].guest_bloggers = _follow_api->get_blog_authors( { acnt } ).blog_authors;
               _state.accounts[acnt].reputation     = _follow_api->get_account_reputations( { acnt, 1 } ).reputations[0].reputation;
            }
            else if( _reputation_api )
            {
               _state.accounts[acnt].reputation    = _reputation_api->get_account_reputations( { acnt, 1 } ).reputations[0].reputation;
            }

            auto& eacnt = _state.accounts[acnt];
            if( part[1] == "transfers" )
            {
               if( _account_history_api )
               {
                  legacy_operation l_op;
                  legacy_operation_conversion_visitor visitor( l_op );
                  auto history = _account_history_api->get_account_history( { acnt, uint64_t(-1), 1000 } ).history;
                  for( auto& item : history )
                  {
                     switch( item.second.op.which() ) {
                        case operation::tag<transfer_to_vesting_operation>::value:
                        case operation::tag<withdraw_vesting_operation>::value:
                        case operation::tag<interest_operation>::value:
                        case operation::tag<transfer_operation>::value:
                        case operation::tag<liquidity_reward_operation>::value:
                        case operation::tag<author_reward_operation>::value:
                        case operation::tag<curation_reward_operation>::value:
                        case operation::tag<comment_benefactor_reward_operation>::value:
                        case operation::tag<transfer_to_savings_operation>::value:
                        case operation::tag<transfer_from_savings_operation>::value:
                        case operation::tag<cancel_transfer_from_savings_operation>::value:
                        case operation::tag<escrow_transfer_operation>::value:
                        case operation::tag<escrow_approve_operation>::value:
                        case operation::tag<escrow_dispute_operation>::value:
                        case operation::tag<escrow_release_operation>::value:
                        case operation::tag<fill_convert_request_operation>::value:
                        case operation::tag<fill_order_operation>::value:
                        case operation::tag<claim_reward_balance_operation>::value:
                           if( item.second.op.visit( visitor ) )
                           {
                              eacnt.transfer_history.emplace( item.first, api_operation_object( item.second, visitor.l_op ) );
                           }
                           break;
                        case operation::tag<comment_operation>::value:
                        //   eacnt.post_history[item.first] =  item.second;
                           break;
                        case operation::tag<limit_order_create_operation>::value:
                        case operation::tag<limit_order_cancel_operation>::value:
                        //   eacnt.market_history[item.first] =  item.second;
                           break;
                        case operation::tag<vote_operation>::value:
                        case operation::tag<account_witness_vote_operation>::value:
                        case operation::tag<account_witness_proxy_operation>::value:
                        //   eacnt.vote_history[item.first] =  item.second;
                           break;
                        case operation::tag<account_create_operation>::value:
                        case operation::tag<account_update_operation>::value:
                        case operation::tag<witness_update_operation>::value:
                        case operation::tag<pow_operation>::value:
                        case operation::tag<custom_operation>::value:
                        case operation::tag<producer_reward_operation>::value:
                        default:
                           if( item.second.op.visit( visitor ) )
                           {
                              eacnt.other_history.emplace( item.first, api_operation_object( item.second, visitor.l_op ) );
                           }
                     }
                  }
               }
            }
            else if( part[1] == "recent-replies" )
            {
               if( _tags_api )
               {
                  auto replies = _tags_api->get_replies_by_last_update( { acnt, "", 50 } ).discussions;
                  eacnt.recent_replies = vector< string >();

                  for( const auto& reply : replies )
                  {
                     string reply_ref = reply.author + "/" + reply.permlink;
                     _state.content[ reply_ref ] = reply;

                     if( _follow_api )
                     {
                        _state.accounts[ reply_ref ].reputation = _follow_api->get_account_reputations( { reply.author, 1 } ).reputations[0].reputation;
                     }
                     else if( _reputation_api )
                     {
                        _state.accounts[ reply_ref ].reputation = _reputation_api->get_account_reputations( { reply.author, 1 } ).reputations[0].reputation;
                     }

                     eacnt.recent_replies->push_back( reply_ref );
                  }
               }
            }
            else if( part[1] == "posts" || part[1] == "comments" )
            {
      #ifndef IS_LOW_MEM
               int count = 0;
               const auto& pidx = _db.get_index< comment_index, by_author_last_update >();
               auto itr = pidx.lower_bound( acnt );
               eacnt.comments = vector<string>();

               while( itr != pidx.end() && itr->author == acnt && count < 20 )
               {
                  if( itr->parent_author.size() )
                  {
                     const auto link = acnt + "/" + to_string( itr->permlink );
                     eacnt.comments->push_back( link );
                     _state.content[ link ] = tags::discussion( *itr, _db );

                     set_pending_payout( _state.content[ link ] );

                     ++count;
                  }

                  ++itr;
               }
      #endif
            }
            else if( part[1].size() == 0 || part[1] == "blog" )
            {
               if( _follow_api )
               {
                  auto blog = _follow_api->get_blog_entries( { eacnt.name, 0, 20 } ).blog;
                  eacnt.blog = vector<string>();
                  eacnt.blog->reserve(blog.size());

                  for( const auto& b: blog )
                  {
                     const auto link = b.author + "/" + b.permlink;
                     eacnt.blog->push_back( link );
                     _state.content[ link ] = tags::discussion( _db.get_comment( b.author, b.permlink ), _db );

                     set_pending_payout( _state.content[ link ] );

                     if( b.reblog_on > time_point_sec() )
                     {
                        _state.content[ link ].first_reblogged_on = b.reblog_on;
                     }
                  }
               }
            }
            else if( part[1].size() == 0 || part[1] == "feed" )
            {
               if( _follow_api )
               {
                  auto feed = _follow_api->get_feed_entries( { eacnt.name, 0, 20 } ).feed;
                  eacnt.feed = vector<string>();
                  eacnt.feed->reserve( feed.size());

                  for( const auto& f: feed )
                  {
                     const auto link = f.author + "/" + f.permlink;
                     eacnt.feed->push_back( link );
                     _state.content[ link ] = tags::discussion( _db.get_comment( f.author, f.permlink ), _db );

                     set_pending_payout( _state.content[ link ] );

                     if( f.reblog_by.empty() == false)
                     {
                        _state.content[link].first_reblogged_by = f.reblog_by[0];
                        _state.content[link].reblogged_by = f.reblog_by;
                        _state.content[link].first_reblogged_on = f.reblog_on;
                     }
                  }
               }
            }
         }
         /// pull a complete discussion
         else if( part[1].size() && part[1][0] == '@' )
         {
            auto account  = part[1].substr( 1 );
            auto slug     = part[2];

            string key = account + "/" + slug;
            if( _tags_api )
            {
               auto dis = _tags_api->get_discussion( { account, slug } );

               recursively_fetch_content( _state, dis, accounts );
               _state.content[key] = std::move(dis);
            }
         }
         else if( part[0] == "witnesses" || part[0] == "~witnesses")
         {
            auto wits = get_witnesses_by_vote( (vector< fc::variant >){ fc::variant(""), fc::variant(100) } );
            for( const auto& w : wits )
            {
               _state.witnesses[w.owner] = w;
            }
         }
         else if( part[0] == "trending"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_trending( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.trending.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "payout"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_post_discussions_by_payout( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.payout.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "payout_comments"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_comment_discussions_by_payout( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.payout_comments.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "promoted" )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_promoted( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.promoted.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "responses"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_children( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.responses.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( !part[0].size() || part[0] == "hot" )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_hot( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.hot.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( !part[0].size() || part[0] == "promoted" )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_promoted( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.promoted.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "votes"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_votes( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.votes.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "cashout"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_cashout( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.cashout.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "active"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_active( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.active.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "created"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_created( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.created.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "recent"  )
         {
            if( _tags_api )
            {
               tags::discussion_query q;
               q.tag = tag;
               q.limit = 20;
               q.truncate_body = 1024;
               auto trending_disc = _tags_api->get_discussions_by_created( q ).discussions;

               auto& didx = _state.discussion_idx[tag];
               for( const auto& d : trending_disc )
               {
                  string key = d.author + "/" + d.permlink;
                  didx.created.push_back( key );
                  if( d.author.size() ) accounts.insert(d.author);
                  _state.content[key] = std::move(d);
               }
            }
         }
         else if( part[0] == "tags" )
         {
            if( _tags_api )
            {
               _state.tag_idx.trending.clear();
               auto trending_tags = _tags_api->get_trending_tags( { std::string(), 250 } ).tags;
               for( const auto& t : trending_tags )
               {
                  string name = t.name;
                  _state.tag_idx.trending.push_back( name );
                  _state.tags[ name ] = api_tag_object( t );
               }
            }
         }
         else {
            elog( "What... no matches" );
         }

         for( const auto& a : accounts )
         {
            _state.accounts.erase("");
            _state.accounts[a] = extended_account( database_api::api_account_object( _db.get_account( a ), _db ) );

            if( _follow_api )
            {
               _state.accounts[a].reputation = _follow_api->get_account_reputations( { a, 1 } ).reputations[0].reputation;
            }
            else if( _reputation_api )
            {
               _state.accounts[a].reputation = _reputation_api->get_account_reputations( { a, 1 } ).reputations[0].reputation;
            }
         }

         for( auto& d : _state.content )
         {
            d.second.active_votes = get_active_votes( { fc::variant( d.second.author ), fc::variant( d.second.permlink ) } );
         }

         _state.witness_schedule = _database_api->get_witness_schedule( {} );

      }
      catch ( const fc::exception& e )
      {
         _state.error = e.to_detail_string();
      }

      return _state;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_active_witnesses )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_active_witnesses( {} ).witnesses;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_block_header )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _block_api, "block_api_plugin not enabled." );
      return _block_api->get_block_header( { args[0].as< uint32_t >() } ).header;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_block )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _block_api, "block_api_plugin not enabled." );
      get_block_return result;
      auto b = _block_api->get_block( { args[0].as< uint32_t >() } ).block;

      if( b )
      {
         result = legacy_signed_block( *b );
         uint32_t n = uint32_t( b->transactions.size() );
         uint32_t block_num = block_header::num_from_id( b->block_id );
         for( uint32_t i=0; i<n; i++ )
         {
            result->transactions[i].transaction_id = b->transactions[i].id();
            result->transactions[i].block_num = block_num;
            result->transactions[i].transaction_num = i;
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_ops_in_block )
   {
      FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

      auto ops = _account_history_api->get_ops_in_block( { args[0].as< uint32_t >(), args[1].as< bool >() } ).ops;
      get_ops_in_block_return result;

      legacy_operation l_op;
      legacy_operation_conversion_visitor visitor( l_op );

      for( auto& op_obj : ops )
      {
         if( op_obj.op.visit( visitor) )
         {
            result.push_back( api_operation_object( op_obj, visitor.l_op ) );
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_config )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_config( {} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_dynamic_global_properties )
   {
      CHECK_ARG_SIZE( 0 )
      get_dynamic_global_properties_return gpo = _database_api->get_dynamic_global_properties( {} );

      return gpo;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_chain_properties )
   {
      CHECK_ARG_SIZE( 0 )
      return api_chain_properties( _database_api->get_witness_schedule( {} ).median_props );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_current_median_history_price )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_current_price_feed( {} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_feed_history )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_feed_history( {} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witness_schedule )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_witness_schedule( {} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_hardfork_version )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_hardfork_properties( {} ).current_hardfork_version;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_next_scheduled_hardfork )
   {
      CHECK_ARG_SIZE( 0 )
      scheduled_hardfork shf;
      const auto& hpo = _db.get( hardfork_property_id_type() );
      shf.hf_version = hpo.next_hardfork;
      shf.live_time = hpo.next_hardfork_time;
      return shf;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_reward_fund )
   {
      CHECK_ARG_SIZE( 1 )
      string name = args[0].as< string >();

      auto fund = _db.find< reward_fund_object, by_name >( name );
      FC_ASSERT( fund != nullptr, "Invalid reward fund name" );

      return api_reward_fund_object( *fund );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_key_references )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _account_by_key_api, "account_history_api_plugin not enabled." );

      return _account_by_key_api->get_key_references( { args[0].as< vector< public_key_type > >() } ).accounts;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_accounts )
   {
      CHECK_ARG_SIZE(1)
      vector< account_name_type > names = args[0].as< vector< account_name_type > >();

      const auto& idx  = _db.get_index< account_index >().indices().get< by_name >();
      const auto& vidx = _db.get_index< witness_vote_index >().indices().get< by_account_witness >();
      vector< extended_account > results;
      results.reserve(names.size());

      for( const auto& name: names )
      {
         auto itr = idx.find( name );
         if ( itr != idx.end() )
         {
            results.emplace_back( extended_account( database_api::api_account_object( *itr, _db ) ) );

            if( _follow_api )
            {
               results.back().reputation = _follow_api->get_account_reputations( { itr->name, 1 } ).reputations[0].reputation;
            }
            else if( _reputation_api )
            {
               results.back().reputation = _reputation_api->get_account_reputations( { itr->name, 1 } ).reputations[0].reputation;
            }

            auto vitr = vidx.lower_bound( boost::make_tuple( itr->name, account_name_type() ) );
            while( vitr != vidx.end() && vitr->account == itr->name ) {
               results.back().witness_votes.insert( _db.get< witness_object, by_name >( vitr->witness ).owner );
               ++vitr;
            }
         }
      }

      return results;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_references )
   {
      FC_ASSERT( false, "condenser_api::get_account_references --- Needs to be refactored for Steem." );
   }

   DEFINE_API_IMPL( condenser_api_impl, lookup_account_names )
   {
      CHECK_ARG_SIZE( 1 )
      vector< account_name_type > account_names = args[0].as< vector< account_name_type > >();

      vector< optional< api_account_object > > result;
      result.reserve( account_names.size() );

      for( auto& name : account_names )
      {
         auto itr = _db.find< account_object, by_name >( name );

         if( itr )
         {
            result.push_back( api_account_object( database_api::api_account_object( *itr, _db ) ) );
         }
         else
         {
            result.push_back( optional< api_account_object >() );
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, lookup_accounts )
   {
      CHECK_ARG_SIZE( 2 )
      account_name_type lower_bound_name = args[0].as< account_name_type >();
      uint32_t limit = args[1].as< uint32_t >();

      FC_ASSERT( limit <= 1000 );
      const auto& accounts_by_name = _db.get_index< account_index, by_name >();
      set<string> result;

      for( auto itr = accounts_by_name.lower_bound( lower_bound_name );
           limit-- && itr != accounts_by_name.end();
           ++itr )
      {
         result.insert( itr->name );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_count )
   {
      CHECK_ARG_SIZE( 0 )
      return _db.get_index<account_index>().indices().size();
   }

   DEFINE_API_IMPL( condenser_api_impl, get_owner_history )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->find_owner_histories( { args[0].as< string >() } ).owner_auths;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_recovery_request )
   {
      CHECK_ARG_SIZE( 1 )
      get_recovery_request_return result;

      auto requests = _database_api->find_account_recovery_requests( { { args[0].as< account_name_type >() } } ).requests;

      if( requests.size() )
         result = requests[0];

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_escrow )
   {
      CHECK_ARG_SIZE( 2 )
      get_escrow_return result;

      auto escrows = _database_api->list_escrows( { { args }, 1, database_api::by_from_id } ).escrows;

      if( escrows.size()
         && escrows[0].from == args[0].as< account_name_type >()
         && escrows[0].escrow_id == args[1].as< uint32_t >() )
      {
         result = escrows[0];
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_withdraw_routes )
   {
      FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );

      auto account = args[0].as< string >();
      auto destination = args.size() == 2 ? args[1].as< withdraw_route_type >() : outgoing;

      get_withdraw_routes_return result;

      if( destination == outgoing || destination == all )
      {
         auto routes = _database_api->find_withdraw_vesting_routes( { account, database_api::by_withdraw_route } ).routes;
         result.insert( result.end(), routes.begin(), routes.end() );
      }

      if( destination == incoming || destination == all )
      {
         auto routes = _database_api->find_withdraw_vesting_routes( { account, database_api::by_destination } ).routes;
         result.insert( result.end(), routes.begin(), routes.end() );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_savings_withdraw_from )
   {
      CHECK_ARG_SIZE( 1 )

      auto withdrawals = _database_api->find_savings_withdrawals(
         {
            args[0].as< string >()
         }).withdrawals;

      get_savings_withdraw_from_return result;

      for( auto& w : withdrawals )
      {
         result.push_back( api_savings_withdraw_object( w ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_savings_withdraw_to )
   {
      CHECK_ARG_SIZE( 1 )
      account_name_type account = args[0].as< account_name_type >();

      get_savings_withdraw_to_return result;

      const auto& to_complete_idx = _db.get_index< savings_withdraw_index, by_to_complete >();
      auto itr = to_complete_idx.lower_bound( account );
      while( itr != to_complete_idx.end() && itr->to == account )
      {
         result.push_back( api_savings_withdraw_object( *itr ) );
         ++itr;
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_vesting_delegations )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );

      database_api::list_vesting_delegations_args a;
      account_name_type account = args[0].as< account_name_type >();
      a.start = fc::variant( (vector< variant >){ args[0], args[1] } );
      a.limit = args.size() == 3 ? args[2].as< uint32_t >() : 100;
      a.order = database_api::by_delegation;

      auto delegations = _database_api->list_vesting_delegations( a ).delegations;
      get_vesting_delegations_return result;

      for( auto itr = delegations.begin(); itr != delegations.end() && itr->delegator == account; ++itr )
      {
         result.push_back( api_vesting_delegation_object( *itr ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_expiring_vesting_delegations )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );

      database_api::list_vesting_delegation_expirations_args a;
      account_name_type account = args[0].as< account_name_type >();
      a.start = fc::variant( (vector< variant >){ args[0], args[1], fc::variant( vesting_delegation_expiration_id_type() ) } );
      a.limit = args.size() == 3 ? args[2].as< uint32_t >() : 100;
      a.order = database_api::by_account_expiration;

      auto delegations = _database_api->list_vesting_delegation_expirations( a ).delegations;
      get_expiring_vesting_delegations_return result;

      for( auto itr = delegations.begin(); itr != delegations.end() && itr->delegator == account; ++itr )
      {
         result.push_back( api_vesting_delegation_expiration_object( *itr ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witnesses )
   {
      CHECK_ARG_SIZE( 1 )
      vector< witness_id_type > witness_ids = args[0].as< vector< witness_id_type > >();

      get_witnesses_return result;
      result.reserve( witness_ids.size() );

      std::transform(
         witness_ids.begin(),
         witness_ids.end(),
         std::back_inserter(result),
         [this](witness_id_type id) -> optional< api_witness_object >
         {
            if( auto o = _db.find(id) )
               return api_witness_object( database_api::api_witness_object ( *o ) );
            return {};
         });

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_conversion_requests )
   {
      CHECK_ARG_SIZE( 1 )
      auto requests = _database_api->find_sbd_conversion_requests(
         {
            args[0].as< account_name_type >()
         }).requests;

      get_conversion_requests_return result;

      for( auto& r : requests )
      {
         result.push_back( api_convert_request_object( r ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witness_by_account )
   {
      CHECK_ARG_SIZE( 1 )
      auto witnesses = _database_api->find_witnesses(
         {
            { args[0].as< account_name_type >() }
         }).witnesses;

      get_witness_by_account_return result;

      if( witnesses.size() )
         result = api_witness_object( witnesses[0] );

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witnesses_by_vote )
   {
      CHECK_ARG_SIZE( 2 )
      account_name_type start_name = args[0].as< account_name_type >();
      vector< fc::variant > start_key;

      if( start_name == account_name_type() )
      {
         start_key.push_back( fc::variant( std::numeric_limits< int64_t >::max() ) );
         start_key.push_back( fc::variant( account_name_type() ) );
      }
      else
      {
         auto start = _database_api->list_witnesses( { args[0], 1, database_api::by_name } );

         if( start.witnesses.size() == 0 )
            return get_witnesses_by_vote_return();

         start_key.push_back( fc::variant( start.witnesses[0].votes ) );
         start_key.push_back( fc::variant( start.witnesses[0].owner ) );
      }

      auto limit = args[1].as< uint32_t >();
      auto witnesses = _database_api->list_witnesses( { fc::variant( start_key ), limit, database_api::by_vote_name } ).witnesses;

      get_witnesses_by_vote_return result;

      for( auto& w : witnesses )
      {
         result.push_back( api_witness_object( w ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, lookup_witness_accounts )
   {
      CHECK_ARG_SIZE( 2 )
      auto limit = args[1].as< uint32_t >();

      lookup_witness_accounts_return result;

      auto witnesses = _database_api->list_witnesses( { args[0], limit, database_api::by_name } ).witnesses;

      for( auto& w : witnesses )
      {
         result.push_back( w.owner );
      }

      return result;
   }
   DEFINE_API_IMPL( condenser_api_impl, get_witness_count )
   {
      CHECK_ARG_SIZE( 0 )
      return _db.get_index< witness_index >().indices().size();
   }

   DEFINE_API_IMPL( condenser_api_impl, get_open_orders )
   {
      CHECK_ARG_SIZE( 1 )
      account_name_type owner = args[0].as< account_name_type >();

      vector< api_limit_order_object > result;
      const auto& idx = _db.get_index< limit_order_index, by_account >();
      auto itr = idx.lower_bound( owner );

      while( itr != idx.end() && itr->seller == owner )
      {
         result.push_back( *itr );

         // if( itr->sell_price.base.symbol == STEEM_SYMBOL )
         //    result.back().real_price = (~result.back().sell_price).to_real();
         // else
         //    result.back().real_price = (result.back().sell_price).to_real();
         result.back().real_price = 0.0;
         ++itr;
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_transaction_hex )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->get_transaction_hex(
      {
         signed_transaction( args[0].as< legacy_signed_transaction >() )
      }).hex;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_transaction )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

      return legacy_signed_transaction( _account_history_api->get_transaction( { args[0].as< transaction_id_type >() } ) );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_required_signatures )
   {
      CHECK_ARG_SIZE( 2 )
      return _database_api->get_required_signatures( {
         signed_transaction( args[0].as< legacy_signed_transaction >() ),
         args[1].as< flat_set< public_key_type > >() } ).keys;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_potential_signatures )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->get_potential_signatures( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } ).keys;
   }

   DEFINE_API_IMPL( condenser_api_impl, verify_authority )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->verify_authority( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } ).valid;
   }

   DEFINE_API_IMPL( condenser_api_impl, verify_account_authority )
   {
      CHECK_ARG_SIZE( 2 )
      return _database_api->verify_account_authority( { args[0].as< account_name_type >(), args[1].as< flat_set< public_key_type > >() } ).valid;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_active_votes )
   {
      CHECK_ARG_SIZE( 2 )

      vector< tags::vote_state > votes;
      const auto& comment = _db.get_comment( args[0].as< account_name_type >(), args[1].as< string >() );
      const auto& idx = _db.get_index< chain::comment_vote_index, chain::by_comment_voter >();
      chain::comment_id_type cid(comment.id);
      auto itr = idx.lower_bound( cid );

      while( itr != idx.end() && itr->comment == cid )
      {
         const auto& vo = _db.get( itr->voter );
         tags::vote_state vstate;
         vstate.voter = vo.name;
         vstate.weight = itr->weight;
         vstate.rshares = itr->rshares;
         vstate.percent = itr->vote_percent;
         vstate.time = itr->last_update;

         if( _follow_api )
         {
            auto reps = _follow_api->get_account_reputations( follow::get_account_reputations_args( { vo.name, 1 } ) ).reputations;
            if( reps.size() )
            {
               vstate.reputation = reps[0].reputation;
            }
         }

         votes.push_back( vstate );
         ++itr;
      }

      return votes;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_votes )
   {
      CHECK_ARG_SIZE( 1 )
      account_name_type voter = args[0].as< account_name_type >();

      vector< account_vote > result;

      const auto& voter_acnt = _db.get_account( voter );
      const auto& idx = _db.get_index< comment_vote_index, by_voter_comment >();

      account_id_type aid( voter_acnt.id );
      auto itr = idx.lower_bound( aid );
      auto end = idx.upper_bound( aid );
      while( itr != end )
      {
         const auto& vo = _db.get( itr->comment );
         account_vote avote;
         avote.authorperm = vo.author + "/" + to_string( vo.permlink );
         avote.weight = itr->weight;
         avote.rshares = itr->rshares;
         avote.percent = itr->vote_percent;
         avote.time = itr->last_update;
         result.push_back( avote );
         ++itr;
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_content )
   {
      CHECK_ARG_SIZE( 2 )

      auto comments = _database_api->find_comments( { { { args[0].as< account_name_type >(), args[1].as< string >() } } } );

      if( comments.comments.size() == 0 )
      {
         return discussion();
      }

      discussion content( comments.comments[0] );
      set_pending_payout( content );
      content.active_votes = get_active_votes( args );

      return content;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_content_replies )
   {
      CHECK_ARG_SIZE( 2 )

      account_name_type author = args[0].as< account_name_type >();
      string permlink = args[1].as< string >();
      const auto& by_permlink_idx = _db.get_index< comment_index, by_parent >();
      auto itr = by_permlink_idx.find( boost::make_tuple( author, permlink ) );
      vector< discussion > result;

      while( itr != by_permlink_idx.end() && itr->parent_author == author && to_string( itr->parent_permlink ) == permlink )
      {
         result.push_back( discussion( database_api::api_comment_object( *itr, _db ) ) );
         set_pending_payout( result.back() );
         ++itr;
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_tags_used_by_author )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      return _tags_api->get_tags_used_by_author( { args[0].as< account_name_type >() } ).tags;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_post_discussions_by_payout )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_post_discussions_by_payout(
         args[0].as< tags::get_post_discussions_by_payout_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_comment_discussions_by_payout )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_comment_discussions_by_payout(
         args[0].as< tags::get_comment_discussions_by_payout_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_trending )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_trending(
         args[0].as< tags::get_discussions_by_trending_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_created )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_created(
         args[0].as< tags::get_discussions_by_created_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_active )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_active(
         args[0].as< tags::get_discussions_by_active_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_cashout )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_cashout(
         args[0].as< tags::get_discussions_by_cashout_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_votes )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_votes(
         args[0].as< tags::get_discussions_by_votes_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_children )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_children(
         args[0].as< tags::get_discussions_by_children_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_hot )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_hot(
         args[0].as< tags::get_discussions_by_hot_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_feed )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_feed(
         args[0].as< tags::get_discussions_by_feed_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_blog )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_blog(
         args[0].as< tags::get_discussions_by_blog_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_comments )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_comments(
         args[0].as< tags::get_discussions_by_comments_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_promoted )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_promoted(
         args[0].as< tags::get_discussions_by_promoted_args >() ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_replies_by_last_update )
   {
      CHECK_ARG_SIZE( 3 )

      vector< discussion > result;

#ifndef IS_LOW_MEM
      account_name_type start_parent_author = args[0].as< account_name_type >();
      string start_permlink = args[1].as< string >();
      uint32_t limit = args[2].as< uint32_t >();

      FC_ASSERT( limit <= 100 );
      const auto& last_update_idx = _db.get_index< comment_index, by_last_update >();
      auto itr = last_update_idx.begin();
      const account_name_type* parent_author = &start_parent_author;

      if( start_permlink.size() )
      {
         const auto& comment = _db.get_comment( start_parent_author, start_permlink );
         itr = last_update_idx.iterator_to( comment );
         parent_author = &comment.parent_author;
      }
      else if( start_parent_author.size() )
      {
         itr = last_update_idx.lower_bound( start_parent_author );
      }

      result.reserve( limit );

      while( itr != last_update_idx.end() && result.size() < limit && itr->parent_author == *parent_author )
      {
         result.push_back( discussion( database_api::api_comment_object( *itr, _db ) ) );
         set_pending_payout( result.back() );
         result.back().active_votes = get_active_votes( { fc::variant( itr->author ), fc::variant( itr->permlink ) } );
         ++itr;
      }

#endif
      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_discussions_by_author_before_date )
   {
      CHECK_ARG_SIZE( 4 )
      FC_ASSERT( _tags_api, "tags_api_plugin not enabled." );

      auto discussions = _tags_api->get_discussions_by_author_before_date( { args[0].as< account_name_type >(), args[1].as< string >(), args[2].as< time_point_sec >(), args[3].as< uint32_t >() } ).discussions;
      vector< discussion > result;

      for( auto& d : discussions )
      {
         result.push_back( discussion( d ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_history )
   {
      CHECK_ARG_SIZE( 3 )
      FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

      auto history = _account_history_api->get_account_history( { args[0].as< account_name_type >(), args[1].as< uint64_t >(), args[2].as< uint32_t >() } ).history;
      get_account_history_return result;

      legacy_operation l_op;
      legacy_operation_conversion_visitor visitor( l_op );

      for( auto& entry : history )
      {
         if( entry.second.op.visit( visitor ) )
         {
            result.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, broadcast_transaction )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );
      return _network_broadcast_api->broadcast_transaction( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } );
   }

   DEFINE_API_IMPL( condenser_api_impl, broadcast_transaction_synchronous )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );
      FC_ASSERT( _p2p != nullptr, "p2p_plugin not enabled." );

      signed_transaction trx = args[0].as< legacy_signed_transaction >();
      auto txid = trx.id();
      boost::promise< broadcast_transaction_synchronous_return > p;

      {
         boost::lock_guard< boost::mutex > guard( _mtx );
         FC_ASSERT( _callbacks.find( txid ) == _callbacks.end(), "Transaction is a duplicate" );
         _callbacks[ txid ] = [&p]( const broadcast_transaction_synchronous_return& r )
         {
            p.set_value( r );
         };
         _callback_expirations[ trx.expiration ].push_back( txid );
      }

      try
      {
         /* It may look strange to call these without the lock and then lock again in the case of an exception,
          * but it is correct and avoids deadlock. accept_transaction is trained along with all other writes, including
          * pushing blocks. Pushing blocks do not originate from this code path and will never have this lock.
          * However, it will trigger the on_post_apply_block callback and then attempt to acquire the lock. In this case,
          * this thread will be waiting on accept_block so it can write and the block thread will be waiting on this
          * thread for the lock.
          */
         _chain.accept_transaction( trx );
         _p2p->broadcast_transaction( trx );
      }
      catch( fc::exception& e )
      {
         boost::lock_guard< boost::mutex > guard( _mtx );

         // The callback may have been cleared in the meantine, so we need to check for existence.
         auto c_itr = _callbacks.find( txid );
         if( c_itr != _callbacks.end() ) _callbacks.erase( c_itr );

         // We do not need to clean up _callback_expirations because on_post_apply_block handles this case.

         throw e;
      }
      catch( ... )
      {
         boost::lock_guard< boost::mutex > guard( _mtx );

         // The callback may have been cleared in the meantine, so we need to check for existence.
         auto c_itr = _callbacks.find( txid );
         if( c_itr != _callbacks.end() ) _callbacks.erase( c_itr );

         throw fc::unhandled_exception(
            FC_LOG_MESSAGE( warn, "Unknown error occured when pushing transaction" ),
            std::current_exception() );
      }

      return p.get_future().get();
   }

   DEFINE_API_IMPL( condenser_api_impl, broadcast_block )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );

      return _network_broadcast_api->broadcast_block( { signed_block( args[0].as< legacy_signed_block >() ) } );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_followers )
   {
      CHECK_ARG_SIZE( 4 )
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_followers( { args[0].as< account_name_type >(), args[1].as< account_name_type >(), args[2].as< follow::follow_type >(), args[3].as< uint32_t >() } ).followers;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_following )
   {
      CHECK_ARG_SIZE( 4 )
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_following( { args[0].as< account_name_type >(), args[1].as< account_name_type >(), args[2].as< follow::follow_type >(), args[3].as< uint32_t >() } ).following;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_follow_count )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_follow_count( { args[0].as< account_name_type >() } );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_feed_entries )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_feed_entries( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).feed;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_feed )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      auto feed = _follow_api->get_feed( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).feed;
      get_feed_return result;
      result.resize( feed.size() );
      result.insert( result.end(), feed.begin(), feed.end() );
      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_blog_entries )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_blog_entries( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).blog;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_blog )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      auto blog = _follow_api->get_blog( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).blog;
      get_blog_return result;
      result.resize( blog.size() );
      result.insert( result.end(), blog.begin(), blog.end() );
      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_reputations )
   {
      FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _follow_api || _reputation_api, "Neither follow_api_plugin nor reputation_api_plugin are enabled. One of these must be running." );

      if( _follow_api )
      {
         return _follow_api->get_account_reputations( { args[0].as< account_name_type >(), args.size() == 2 ? args[1].as< uint32_t >() : 1000 } ).reputations;
      }
      else
      {
         return _reputation_api->get_account_reputations( { args[0].as< account_name_type >(), args.size() == 2 ? args[1].as< uint32_t >() : 1000 } ).reputations;
      }
   }

   DEFINE_API_IMPL( condenser_api_impl, get_reblogged_by )
   {
      CHECK_ARG_SIZE( 2 )
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_reblogged_by( { args[0].as< account_name_type >(), args[1].as< string >() } ).accounts;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_blog_authors )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _follow_api, "follow_api_plugin not enabled." );

      return _follow_api->get_blog_authors( { args[0].as< account_name_type >() } ).blog_authors;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_ticker )
   {
      CHECK_ARG_SIZE( 0 )
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      return get_ticker_return( _market_history_api->get_ticker( {} ) );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_volume )
   {
      CHECK_ARG_SIZE( 0 )
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      return get_volume_return( _market_history_api->get_volume( {} ) );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_order_book )
   {
      FC_ASSERT( args.size() == 0 || args.size() == 1, "Expected 0-1 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      return get_order_book_return( _market_history_api->get_order_book( { args.size() == 1 ? args[0].as< uint32_t >() : 500 } ) );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_trade_history )
   {
      FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      const auto& trades = _market_history_api->get_trade_history( { args[0].as< time_point_sec >(), args[1].as< time_point_sec >(), args.size() == 3 ? args[2].as< uint32_t >() : 1000 } ).trades;
      get_trade_history_return result;

      for( const auto& t : trades ) result.push_back( market_trade( t ) );

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_recent_trades )
   {
      FC_ASSERT( args.size() == 0 || args.size() == 1, "Expected 0-1 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      const auto& trades = _market_history_api->get_recent_trades( { args.size() == 1 ? args[0].as< uint32_t >() : 1000 } ).trades;
      get_trade_history_return result;

      for( const auto& t : trades ) result.push_back( market_trade( t ) );

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_market_history )
   {
      CHECK_ARG_SIZE( 3 )
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      return _market_history_api->get_market_history( { args[0].as< uint32_t >(), args[1].as< time_point_sec >(), args[2].as< time_point_sec >() } ).buckets;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_market_history_buckets )
   {
      CHECK_ARG_SIZE( 0 )
      FC_ASSERT( _market_history_api, "market_history_api_plugin not enabled." );

      return _market_history_api->get_market_history_buckets( {} ).bucket_sizes;
   }

   /**
    *  This call assumes root already stored as part of state, it will
    *  modify root.replies to contain links to the reply posts and then
    *  add the reply discussions to the state. This method also fetches
    *  any accounts referenced by authors.
    *
    */
   void condenser_api_impl::recursively_fetch_content( state& _state, tags::discussion& root, set<string>& referenced_accounts )
   {
      try
      {
         if( root.author.size() )
            referenced_accounts.insert( root.author );

         if( _tags_api )
         {
            auto replies = _tags_api->get_content_replies( { root.author, root.permlink } ).discussions;
            for( auto& r : replies )
            {
               try
               {
                  recursively_fetch_content( _state, r, referenced_accounts );
                  root.replies.push_back( r.author + "/" + r.permlink  );
                  _state.content[r.author + "/" + r.permlink] = std::move( r );

                  if( r.author.size() )
                     referenced_accounts.insert( r.author );
               }
               catch( const fc::exception& e )
               {
                  edump( (e.to_detail_string()) );
               }
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (root.author)(root.permlink) )
   }

   void condenser_api_impl::set_pending_payout( discussion& d )
   {
      if( _tags_api )
      {
         const auto& cidx = _db.get_index< tags::tag_index, tags::by_comment>();
         auto itr = cidx.lower_bound( d.id );
         if( itr != cidx.end() && itr->comment == d.id )
         {
            d.promoted = legacy_asset::from_asset( asset( itr->promoted_balance, SBD_SYMBOL ) );
         }
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

         d.pending_payout_value = legacy_asset::from_asset( asset( static_cast<uint64_t>(r2), pot.symbol ) );

         if( _follow_api )
         {
            d.author_reputation = _follow_api->get_account_reputations( follow::get_account_reputations_args( { d.author, 1} ) ).reputations[0].reputation;
         }
         else if( _reputation_api )
         {
            d.author_reputation = _reputation_api->get_account_reputations( reputation::get_account_reputations_args( { d.author, 1} ) ).reputations[0].reputation;
         }
      }

      if( d.parent_author != STEEM_ROOT_POST_PARENT )
         d.cashout_time = _db.calculate_discussion_payout_time( _db.get< chain::comment_object >( d.id ) );

      if( d.body.size() > 1024*128 )
         d.body = "body pruned due to size";
      if( d.parent_author.size() > 0 && d.body.size() > 1024*16 )
         d.body = "comment pruned due to size";

      const database_api::api_comment_object root( _db.get_comment( d.root_author, d.root_permlink ), _db );
      d.url = "/" + root.category + "/@" + root.author + "/" + root.permlink;
      d.root_title = root.title;
      if( root.id != d.id )
         d.url += "#@" + d.author + "/" + d.permlink;
   }

   void condenser_api_impl::on_post_apply_block( const signed_block& b )
   { try {
      boost::lock_guard< boost::mutex > guard( _mtx );
      int32_t block_num = int32_t(b.block_num());
      if( _callbacks.size() )
      {
         for( size_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
         {
            const auto& trx = b.transactions[trx_num];
            auto id = trx.id();
            auto itr = _callbacks.find( id );
            if( itr == _callbacks.end() ) continue;
            itr->second( broadcast_transaction_synchronous_return( id, block_num, int32_t( trx_num ), false ) );
            _callbacks.erase( itr );
         }
      }

      /// clear all expirations
      while( true )
      {
         auto exp_it = _callback_expirations.begin();
         if( exp_it == _callback_expirations.end() )
            break;
         if( exp_it->first >= b.timestamp )
            break;
         for( const transaction_id_type& txid : exp_it->second )
         {
            auto cb_it = _callbacks.find( txid );
            // If it's empty, that means the transaction has been confirmed and has been deleted by the above check.
            if( cb_it == _callbacks.end() )
               continue;

            confirmation_callback callback = cb_it->second;
            transaction_id_type txid_byval = txid;    // can't pass in by reference as it's going to be deleted
            callback( broadcast_transaction_synchronous_return( txid_byval, block_num, -1, true ) );

            _callbacks.erase( cb_it );
         }
         _callback_expirations.erase( exp_it );
      }
   } FC_LOG_AND_RETHROW() }

} // detail

uint16_t api_account_object::_compute_voting_power( const database_api::api_account_object& a )
{
   if( a.voting_manabar.last_update_time < STEEM_HARDFORK_0_20_TIME )
      return (uint16_t) a.voting_manabar.current_mana;

   auto vests = chain::util::get_effective_vesting_shares( a );
   if( vests <= 0 )
      return 0;

   //
   // Let t1 = last_vote_time, t2 = last_update_time
   // vp_t2 = STEEM_100_PERCENT * current_mana / vests
   // vp_t1 = vp_t2 - STEEM_100_PERCENT * (t2 - t1) / STEEM_VOTING_MANA_REGENERATION_SECONDS
   //

   uint32_t t1 = a.last_vote_time.sec_since_epoch();
   uint32_t t2 = a.voting_manabar.last_update_time;
   uint64_t dt = (t2 > t1) ? (t2 - t1) : 0;
   uint64_t vp_dt = STEEM_100_PERCENT * dt / STEEM_VOTING_MANA_REGENERATION_SECONDS;

   uint128_t vp_t2 = STEEM_100_PERCENT;
   vp_t2 *= a.voting_manabar.current_mana;
   vp_t2 /= vests;

   uint64_t vp_t2u = vp_t2.to_uint64();
   if( vp_t2u >= STEEM_100_PERCENT )
   {
      wlog( "Truncated vp_t2u to STEEM_100_PERCENT for account ${a}", ("a", a.name) );
      vp_t2u = STEEM_100_PERCENT;
   }
   uint16_t vp_t1 = uint16_t( vp_t2u ) - uint16_t( std::min( vp_t2u, vp_dt ) );

   return vp_t1;
}

condenser_api::condenser_api()
   : my( new detail::condenser_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_CONDENSER_API_PLUGIN_NAME );
}

condenser_api::~condenser_api() {}

void condenser_api::api_startup()
{
   auto database = appbase::app().find_plugin< database_api::database_api_plugin >();
   if( database != nullptr )
   {
      my->_database_api = database->api;
   }

   auto block = appbase::app().find_plugin< block_api::block_api_plugin >();
   if( block != nullptr )
   {
      my->_block_api = block->api;
   }

   auto account_by_key = appbase::app().find_plugin< account_by_key::account_by_key_api_plugin >();
   if( account_by_key != nullptr )
   {
      my->_account_by_key_api = account_by_key->api;
   }

   auto account_history = appbase::app().find_plugin< account_history::account_history_api_plugin >();
   if( account_history != nullptr )
   {
      my->_account_history_api = account_history->api;
   }

   auto network_broadcast = appbase::app().find_plugin< network_broadcast_api::network_broadcast_api_plugin >();
   if( network_broadcast != nullptr )
   {
      my->_network_broadcast_api = network_broadcast->api;
   }

   auto p2p = appbase::app().find_plugin< p2p::p2p_plugin >();
   if( p2p != nullptr )
   {
      my->_p2p = p2p;
   }

   auto tags = appbase::app().find_plugin< tags::tags_api_plugin >();
   if( tags != nullptr )
   {
      my->_tags_api = tags->api;
   }

   auto follow = appbase::app().find_plugin< follow::follow_api_plugin >();
   if( follow != nullptr )
   {
      my->_follow_api = follow->api;
   }

   auto reputation = appbase::app().find_plugin< reputation::reputation_api_plugin >();
   if( reputation != nullptr )
   {
      my->_reputation_api = reputation->api;
   }

   auto market_history = appbase::app().find_plugin< market_history::market_history_api_plugin >();
   if( market_history != nullptr )
   {
      my->_market_history_api = market_history->api;
   }
}

DEFINE_LOCKLESS_APIS( condenser_api,
   (get_version)
   (get_config)
   (get_account_references)
   (broadcast_transaction)
   (broadcast_transaction_synchronous)
   (broadcast_block)
   (get_market_history_buckets)
)

DEFINE_READ_APIS( condenser_api,
   (get_trending_tags)
   (get_state)
   (get_active_witnesses)
   (get_block_header)
   (get_block)
   (get_ops_in_block)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_current_median_history_price)
   (get_feed_history)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)
   (get_reward_fund)
   (get_key_references)
   (get_accounts)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_owner_history)
   (get_recovery_request)
   (get_escrow)
   (get_withdraw_routes)
   (get_savings_withdraw_from)
   (get_savings_withdraw_to)
   (get_vesting_delegations)
   (get_expiring_vesting_delegations)
   (get_witnesses)
   (get_conversion_requests)
   (get_witness_by_account)
   (get_witnesses_by_vote)
   (lookup_witness_accounts)
   (get_witness_count)
   (get_open_orders)
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)
   (get_active_votes)
   (get_account_votes)
   (get_content)
   (get_content_replies)
   (get_tags_used_by_author)
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
   (get_account_history)
   (get_followers)
   (get_following)
   (get_follow_count)
   (get_feed_entries)
   (get_feed)
   (get_blog_entries)
   (get_blog)
   (get_account_reputations)
   (get_reblogged_by)
   (get_blog_authors)
   (get_ticker)
   (get_volume)
   (get_order_book)
   (get_trade_history)
   (get_recent_trades)
   (get_market_history)
)

} } } // steem::plugins::condenser_api
