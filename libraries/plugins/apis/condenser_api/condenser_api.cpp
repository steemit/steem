#include <steem/plugins/condenser_api/condenser_api.hpp>
#include <steem/plugins/condenser_api/condenser_api_plugin.hpp>

#include <steem/plugins/database_api/database_api_plugin.hpp>
#include <steem/plugins/block_api/block_api_plugin.hpp>
#include <steem/plugins/account_history_api/account_history_api_plugin.hpp>
#include <steem/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <steem/plugins/tags_api/tags_api_plugin.hpp>
#include <steem/plugins/follow_api/follow_api_plugin.hpp>
#include <steem/plugins/market_history_api/market_history_api_plugin.hpp>
#include <steem/plugins/witness_api/witness_api_plugin.hpp>

#include <steem/utilities/git_revision.hpp>

#include <fc/git_revision.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#define CHECK_ARG_SIZE( s ) \
   FC_ASSERT( args.size() == s, "Expected #s argument(s), was ${n}", ("n", args.size()) );

namespace steem { namespace plugins { namespace condenser_api {

namespace detail
{
   class condenser_api_impl
   {
      public:
         condenser_api_impl() : _db( appbase::app().get_plugin< chain::chain_plugin >().db() ) {}

         DECLARE_API(
            (get_state)
            (get_next_scheduled_hardfork)
            (get_reward_fund)
            (get_accounts)
            (lookup_account_names)
            (lookup_accounts)
            (get_account_count)
            (get_savings_withdraw_to)
            (get_witnesses)
            (get_witness_count)
            (get_open_orders)
            (get_account_votes)
            (lookup_witness_accounts)
         )

         void recursively_fetch_content( state& _state, tags::discussion& root, set<string>& referenced_accounts );

         chain::database& _db;

         std::shared_ptr< database_api::database_api > _database_api;
         std::shared_ptr< block_api::block_api > _block_api;
         std::shared_ptr< account_history::account_history_api > _account_history_api;
         std::shared_ptr< account_by_key::account_by_key_api > _account_by_key_api;
         std::shared_ptr< network_broadcast_api::network_broadcast_api > _network_broadcast_api;
         std::shared_ptr< tags::tags_api > _tags_api;
         std::shared_ptr< follow::follow_api > _follow_api;
         std::shared_ptr< market_history::market_history_api > _market_history_api;
         std::shared_ptr< witness::witness_api > _witness_api;
   };

   DEFINE_API( condenser_api_impl, get_state )
   {
      string path = args[0].as< string >();

      state _state;
      _state.props         = _database_api->get_dynamic_global_properties( {} );
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
            auto trending_tags = _tags_api->get_trending_tags( tags::get_trending_tags_args( { std::string(), 50 } ) ).tags;
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
            _state.accounts[acnt] = extended_account( _db.get_account( acnt ), _db );

            if( _tags_api )
               _state.accounts[acnt].tags_usage = _tags_api->get_tags_used_by_author( tags::get_tags_used_by_author_args( { acnt } ) ).tags;

            if( _follow_api )
            {
               _state.accounts[acnt].guest_bloggers = _follow_api->get_blog_authors( follow::get_blog_authors_args( { acnt } ) ).blog_authors;
               _state.accounts[acnt].reputation     = _follow_api->get_account_reputations( follow::get_account_reputations_args( { acnt, 1 } ) ).reputations[0].reputation;
            }

            auto& eacnt = _state.accounts[acnt];
            if( part[1] == "transfers" )
            {
               if( _account_history_api )
               {
                  auto history = _account_history_api->get_account_history( account_history::get_account_history_args( { acnt, uint64_t(-1), 1000 } ) ).history;
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
                           eacnt.transfer_history[item.first] =  item.second;
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
                           eacnt.other_history[item.first] =  item.second;
                     }
                  }
               }
            }
            else if( part[1] == "recent-replies" )
            {
               if( _tags_api )
               {
                  auto replies = _tags_api->get_replies_by_last_update( tags::get_replies_by_last_update_args( { acnt, "", 50 } ) ).discussions;
                  eacnt.recent_replies = vector< string >();

                  for( const auto& reply : replies )
                  {
                     auto reply_ref = reply.author+"/"+reply.permlink;
                     _state.content[ reply_ref ] = reply;

                     if( _follow_api )
                     {
                        _state.accounts[ reply_ref ].reputation = _follow_api->get_account_reputations( follow::get_account_reputations_args( { reply.author, 1 } ) ).reputations[0].reputation;
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

                     if( _tags_api )
                        _tags_api->set_pending_payout( _state.content[ link ] );

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
                  auto blog = _follow_api->get_blog_entries( follow::get_blog_entries_args( { eacnt.name, 0, 20 } ) ).blog;
                  eacnt.blog = vector<string>();
                  eacnt.blog->reserve(blog.size());

                  for( const auto& b: blog )
                  {
                     const auto link = b.author + "/" + b.permlink;
                     eacnt.blog->push_back( link );
                     _state.content[ link ] = tags::discussion( _db.get_comment( b.author, b.permlink ), _db );

                     if( _tags_api )
                        _tags_api->set_pending_payout( _state.content[ link ] );

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
                  auto feed = _follow_api->get_feed_entries( follow::get_feed_entries_args( { eacnt.name, 0, 20 } ) ).feed;
                  eacnt.feed = vector<string>();
                  eacnt.feed->reserve( feed.size());

                  for( const auto& f: feed )
                  {
                     const auto link = f.author + "/" + f.permlink;
                     eacnt.feed->push_back( link );
                     _state.content[ link ] = tags::discussion( _db.get_comment( f.author, f.permlink ), _db );

                     if( _tags_api )
                        _tags_api->set_pending_payout( _state.content[ link ] );

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

            auto key = account + "/" + slug;
            if( _tags_api )
            {
               auto dis = _tags_api->get_discussion( tags::get_discussion_args( { account, slug } ) );

               recursively_fetch_content( _state, dis, accounts );
               _state.content[key] = std::move(dis);
            }
         }
         else if( part[0] == "witnesses" || part[0] == "~witnesses")
         {
            auto start = _database_api->list_witnesses(
               database_api::list_witnesses_args( { { "" }, 1, database_api::by_name } ) );

            if( BOOST_LIKELY( start.witnesses.size() ) )
            {
               vector< variant > start_key;
               start_key.push_back( fc::variant( start.witnesses[0].votes ) );
               start_key.push_back( fc::variant( start.witnesses[0].owner ) );

               auto wits = _database_api->list_witnesses(
                  database_api::list_witnesses_args( { fc::variant( start_key ), 50, database_api::by_vote_name } ) ).witnesses;

               //auto wits = get_witnesses_by_vote( "", 50 );
               for( const auto& w : wits )
               {
                  _state.witnesses[w.owner] = w;
               }
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author + "/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
                  auto key = d.author +"/" + d.permlink;
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
               auto trending_tags = _tags_api->get_trending_tags( tags::get_trending_tags_args( { std::string(), 250 } ) ).tags;
               for( const auto& t : trending_tags )
               {
                  string name = t.name;
                  _state.tag_idx.trending.push_back( name );
                  _state.tags[ name ] = t;
               }
            }
         }
         else {
            elog( "What... no matches" );
         }

         for( const auto& a : accounts )
         {
            _state.accounts.erase("");
            _state.accounts[a] = extended_account( _db.get_account( a ), _db );

            if( _follow_api )
            {
               _state.accounts[a].reputation = _follow_api->get_account_reputations( follow::get_account_reputations_args( { a, 1 } ) ).reputations[0].reputation;
            }
         }
         if( _tags_api )
         {
            for( auto& d : _state.content )
            {
               d.second.active_votes = _tags_api->get_active_votes( tags::get_active_votes_args( { d.second.author, d.second.permlink } ) ).votes;
            }
         }

         _state.witness_schedule = _db.get_witness_schedule_object();

      }
      catch ( const fc::exception& e )
      {
         _state.error = e.to_detail_string();
      }

      return _state;
   }

   DEFINE_API( condenser_api_impl, get_next_scheduled_hardfork )
   {
      scheduled_hardfork shf;
      const auto& hpo = _db.get( hardfork_property_id_type() );
      shf.hf_version = hpo.next_hardfork;
      shf.live_time = hpo.next_hardfork_time;
      return shf;
   }

   DEFINE_API( condenser_api_impl, get_reward_fund )
   {
      string name = args[0].as< string >();

      auto fund = _db.find< reward_fund_object, by_name >( name );
      FC_ASSERT( fund != nullptr, "Invalid reward fund name" );

      return *fund;
   }

   DEFINE_API( condenser_api_impl, get_accounts )
   {
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
            results.emplace_back( extended_account( *itr, _db ) );

            if( _follow_api )
            {
               results.back().reputation = _follow_api->get_account_reputations( follow::get_account_reputations_args( { itr->name, 1 } ) ).reputations[0].reputation;
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

   DEFINE_API( condenser_api_impl, lookup_account_names )
   {
      vector< account_name_type > account_names = args[0].as< vector< account_name_type > >();

      vector< optional< database_api::api_account_object > > result;
      result.reserve( account_names.size() );

      for( auto& name : account_names )
      {
         auto itr = _db.find< account_object, by_name >( name );

         if( itr )
         {
            result.push_back( database_api::api_account_object( *itr, _db ) );
         }
         else
         {
            result.push_back( optional< database_api::api_account_object >() );
         }
      }

      return result;
   }

   DEFINE_API( condenser_api_impl, lookup_accounts )
   {
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

   DEFINE_API( condenser_api_impl, get_account_count )
   {
      return _db.get_index<account_index>().indices().size();
   }

   DEFINE_API( condenser_api_impl, get_savings_withdraw_to )
   {
      CHECK_ARG_SIZE( 1 )
      account_name_type account = args[0].as< account_name_type >();

      vector< database_api::api_savings_withdraw_object > result;

      const auto& to_complete_idx = _db.get_index< savings_withdraw_index, by_to_complete >();
      auto itr = to_complete_idx.lower_bound( account );
      while( itr != to_complete_idx.end() && itr->to == account )
      {
         result.push_back( database_api::api_savings_withdraw_object( *itr ) );
         ++itr;
      }

      return result;
   }

   DEFINE_API( condenser_api_impl, get_witnesses )
   {
      vector< witness_id_type > witness_ids = args[0].as< vector< witness_id_type > >();

      vector< optional< database_api::api_witness_object > > result;
      result.reserve( witness_ids.size() );

      std::transform(
         witness_ids.begin(),
         witness_ids.end(),
         std::back_inserter(result),
         [this](witness_id_type id) -> optional< database_api::api_witness_object >
         {
            if( auto o = _db.find(id) )
               return *o;
            return {};
         });

      return result;
   }

   DEFINE_API( condenser_api_impl, get_witness_count )
   {
      return _db.get_index< witness_index >().indices().size();
   }

   DEFINE_API( condenser_api_impl, get_open_orders )
   {
      account_name_type owner = args[0].as< account_name_type >();

      vector< extended_limit_order > result;
      const auto& idx = _db.get_index< limit_order_index, by_account >();
      auto itr = idx.lower_bound( owner );

      while( itr != idx.end() && itr->seller == owner )
      {
         result.push_back( *itr );

         if( itr->sell_price.base.symbol == STEEM_SYMBOL )
            result.back().real_price = (~result.back().sell_price).to_real();
         else
            result.back().real_price = (result.back().sell_price).to_real();
         ++itr;
      }

      return result;
   }

   DEFINE_API( condenser_api_impl, get_account_votes )
   {
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

   DEFINE_API( condenser_api_impl, lookup_witness_accounts )
   {
      auto limit = args[1].as< uint32_t >();

      lookup_witness_accounts_return result;

      auto witnesses = _database_api->list_witnesses( { args[0], limit, database_api::by_name } ).witnesses;

      for( auto& w : witnesses )
      {
         result.push_back( w.owner );
      }

      return result;
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
            auto replies = _tags_api->get_content_replies( tags::get_content_replies_args( { root.author, root.permlink } ) ).discussions;
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

} // detail

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
      my->_database_api = database->api;

   auto block = appbase::app().find_plugin< block_api::block_api_plugin >();
   if( block != nullptr )
      my->_block_api = block->api;

   auto account_by_key = appbase::app().find_plugin< account_by_key::account_by_key_api_plugin >();
   if( account_by_key != nullptr )
      my->_account_by_key_api = account_by_key->api;

   auto account_history = appbase::app().find_plugin< account_history::account_history_api_plugin >();
   if( account_history != nullptr )
      my->_account_history_api = account_history->api;

   auto network_broadcast = appbase::app().find_plugin< network_broadcast_api::network_broadcast_api_plugin >();
   if( network_broadcast != nullptr )
      my->_network_broadcast_api = network_broadcast->api;

   auto tags = appbase::app().find_plugin< tags::tags_api_plugin >();
   if( tags != nullptr )
      my->_tags_api = tags->api;

   auto follow = appbase::app().find_plugin< follow::follow_api_plugin >();
   if( follow != nullptr )
      my->_follow_api = follow->api;

   auto market_history = appbase::app().find_plugin< market_history::market_history_api_plugin >();
   if( market_history != nullptr )
      my->_market_history_api = market_history->api;

   auto witness = appbase::app().find_plugin< witness::witness_api_plugin >();
   if( witness != nullptr )
      my->_witness_api = witness->api;
}

DEFINE_API( condenser_api, get_version )
{
   CHECK_ARG_SIZE( 0 )
   return get_version_return
   (
      fc::string( STEEM_BLOCKCHAIN_VERSION ),
      fc::string( steem::utilities::git_revision_sha ),
      fc::string( fc::git_revision_sha )
   );
}

DEFINE_API( condenser_api, get_trending_tags )
{
   CHECK_ARG_SIZE( 2 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_trending_tags( { args[0].as< string >(), args[1].as< uint32_t >() } ).tags;
}

DEFINE_API( condenser_api, get_state )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_state( args );
   });
}

DEFINE_API( condenser_api, get_active_witnesses )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_active_witnesses( {} ).witnesses;
}

DEFINE_API( condenser_api, get_block_header )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_block_api, "block_api_plugin not enabled." );
   return my->_block_api->get_block_header( { args[0].as< uint32_t >() } ).header;
}

DEFINE_API( condenser_api, get_block )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_block_api, "block_api_plugin not enabled." );
   return my->_block_api->get_block( { args[0].as< uint32_t >() } ).block;
}

DEFINE_API( condenser_api, get_ops_in_block )
{
   FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_account_history_api, "account_history_api_plugin not enabled." );

   return my->_account_history_api->get_ops_in_block( { args[0].as< uint32_t >(), args[1].as< bool >() } ).ops;
}

DEFINE_API( condenser_api, get_config )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_config( {} );
}

DEFINE_API( condenser_api, get_dynamic_global_properties )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_dynamic_global_properties( {} );
}

DEFINE_API( condenser_api, get_chain_properties )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_witness_schedule( {} ).median_props;
}

DEFINE_API( condenser_api, get_current_median_history_price )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_current_price_feed( {} );
}

DEFINE_API( condenser_api, get_feed_history )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_feed_history( {} );
}

DEFINE_API( condenser_api, get_witness_schedule )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_witness_schedule( {} );
}

DEFINE_API( condenser_api, get_hardfork_version )
{
   CHECK_ARG_SIZE( 0 )
   return my->_database_api->get_hardfork_properties( {} ).current_hardfork_version;
}

DEFINE_API( condenser_api, get_next_scheduled_hardfork )
{
   CHECK_ARG_SIZE( 0 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_next_scheduled_hardfork( args );
   });
}

DEFINE_API( condenser_api, get_reward_fund )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_reward_fund( args );
   });
}

DEFINE_API( condenser_api, get_key_references )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_account_by_key_api, "account_history_api_plugin not enabled." );

   return my->_account_by_key_api->get_key_references( { args[0].as< vector< public_key_type > >() } ).accounts;
}

DEFINE_API( condenser_api, get_accounts )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_accounts( args );
   });
}

DEFINE_API( condenser_api, get_account_references )
{
   FC_ASSERT( false, "condenser_api::get_account_references --- Needs to be refactored for Steem." );
}

DEFINE_API( condenser_api, lookup_account_names )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_account_names( args );
   });
}

DEFINE_API( condenser_api, lookup_accounts )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_accounts( args );
   });
}

DEFINE_API( condenser_api, get_account_count )
{
   CHECK_ARG_SIZE( 0 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_count( args );
   });
}

DEFINE_API( condenser_api, get_owner_history )
{
   CHECK_ARG_SIZE( 1 )
   return my->_database_api->find_owner_histories( { args[0].as< string >() } ).owner_auths;
}

DEFINE_API( condenser_api, get_recovery_request )
{
   CHECK_ARG_SIZE( 1 )
   get_recovery_request_return result;

   auto requests = my->_database_api->find_account_recovery_requests( { { args[0].as< account_name_type >() } } ).requests;

   if( requests.size() )
      result = requests[0];

   return result;
}

DEFINE_API( condenser_api, get_escrow )
{
   CHECK_ARG_SIZE( 2 )
   get_escrow_return result;

   auto escrows = my->_database_api->list_escrows( { { args }, 1, database_api::by_from_id } ).escrows;

   if( escrows.size()
      && escrows[0].from == args[0].as< account_name_type >()
      && escrows[0].escrow_id == args[1].as< uint32_t >() )
   {
      result = escrows[0];
   }

   return result;
}

DEFINE_API( condenser_api, get_withdraw_routes )
{
   FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );

   auto account = args[0].as< string >();
   auto destination = args.size() == 2 ? args[1].as< withdraw_route_type >() : outgoing;

   get_withdraw_routes_return result;

   if( destination == outgoing || destination == all )
   {
      auto routes = my->_database_api->find_withdraw_vesting_routes( { account, database_api::by_withdraw_route } ).routes;
      result.insert( result.end(), routes.begin(), routes.end() );
   }

   if( destination == incoming || destination == all )
   {
      auto routes = my->_database_api->find_withdraw_vesting_routes( { account, database_api::by_destination } ).routes;
      result.insert( result.end(), routes.begin(), routes.end() );
   }

   return result;
}

DEFINE_API( condenser_api, get_account_bandwidth )
{
   CHECK_ARG_SIZE( 2 )
   FC_ASSERT( my->_witness_api, "witness_api_plugin not enabled." );
   return my->_witness_api->get_account_bandwidth(
      witness::get_account_bandwidth_args({
         args[0].as< string >(),
         args[1].as< witness::bandwidth_type >()
      })).bandwidth;
}

DEFINE_API( condenser_api, get_savings_withdraw_from )
{
   CHECK_ARG_SIZE( 1 )
   return my->_database_api->find_savings_withdrawals(
      database_api::find_savings_withdrawals_args({
         args[0].as< string >()
      })).withdrawals;
}

DEFINE_API( condenser_api, get_savings_withdraw_to )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_savings_withdraw_to( args );
   });
}

DEFINE_API( condenser_api, get_vesting_delegations )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );

   database_api::list_vesting_delegations_args a;
   a.start = fc::variant( (vector< variant >){ args[0], args[1] } );
   a.limit = args.size() == 3 ? args[2].as< uint32_t >() : 100;
   a.order = database_api::by_delegation;

   return my->_database_api->list_vesting_delegations( a ).delegations;
}

DEFINE_API( condenser_api, get_expiring_vesting_delegations )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );

   database_api::list_vesting_delegation_expirations_args a;
   a.start = fc::variant( (vector< variant >){ args[0], args[1] } );
   a.limit = args.size() == 3 ? args[2].as< uint32_t >() : 100;
   a.order = database_api::by_account_expiration;

   return my->_database_api->list_vesting_delegation_expirations( a ).delegations;
}

DEFINE_API( condenser_api, get_witnesses )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_witnesses( args );
   });
}

DEFINE_API( condenser_api, get_conversion_requests )
{
   CHECK_ARG_SIZE( 1 )
   return my->_database_api->find_sbd_conversion_requests(
      database_api::find_sbd_conversion_requests_args({
         args[0].as< account_name_type >()
      })).requests;
}

DEFINE_API( condenser_api, get_witness_by_account )
{
   CHECK_ARG_SIZE( 1 )
   auto witnesses = my->_database_api->find_witnesses(
      database_api::find_witnesses_args({
         { args[0].as< account_name_type >() }
      })).witnesses;

   get_witness_by_account_return result;

   if( witnesses.size() )
      result = witnesses[0];

   return result;
}

DEFINE_API( condenser_api, get_witnesses_by_vote )
{
   CHECK_ARG_SIZE( 2 )
   auto start = my->_database_api->list_witnesses( { args[0], 1, database_api::by_name } );

   if( start.witnesses.size() == 0 )
      return start.witnesses;

   auto limit = args[1].as< uint32_t >();
   vector< fc::variant > start_key;
   start_key.push_back( fc::variant( start.witnesses[0].votes ) );
   start_key.push_back( fc::variant( start.witnesses[0].owner ) );

   return my->_database_api->list_witnesses( { fc::variant( start_key ), limit, database_api::by_vote_name } ).witnesses;
}

DEFINE_API( condenser_api, lookup_witness_accounts )
{
   CHECK_ARG_SIZE( 2 )
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_witness_accounts( args );
   });
}

DEFINE_API( condenser_api, get_witness_count )
{
   CHECK_ARG_SIZE( 0 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_witness_count( args );
   });
}

DEFINE_API( condenser_api, get_open_orders )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_open_orders( args );
   });
}

DEFINE_API( condenser_api, get_transaction_hex )
{
   CHECK_ARG_SIZE( 1 )
   return my->_database_api->get_transaction_hex(
      database_api::get_transaction_hex_args({
         args[0].as< signed_transaction >()
      })).hex;
}

DEFINE_API( condenser_api, get_transaction )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_account_history_api, "account_history_api_plugin not enabled." );

   return my->_account_history_api->get_transaction( { args[0].as< transaction_id_type >() } );
}

DEFINE_API( condenser_api, get_required_signatures )
{
   CHECK_ARG_SIZE( 2 )
   return my->_database_api->get_required_signatures( { args[0].as< signed_transaction >(), args[1].as< flat_set< public_key_type > >() } ).keys;
}

DEFINE_API( condenser_api, get_potential_signatures )
{
   CHECK_ARG_SIZE( 1 )
   return my->_database_api->get_potential_signatures( { args[0].as< signed_transaction >() } ).keys;
}

DEFINE_API( condenser_api, verify_authority )
{
   CHECK_ARG_SIZE( 1 )
   return my->_database_api->verify_authority( { args[0].as< signed_transaction >() } ).valid;
}

DEFINE_API( condenser_api, verify_account_authority )
{
   CHECK_ARG_SIZE( 2 )
   return my->_database_api->verify_account_authority( { args[0].as< account_name_type >(), args[1].as< flat_set< public_key_type > >() } ).valid;
}

DEFINE_API( condenser_api, get_active_votes )
{
   CHECK_ARG_SIZE( 2 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_active_votes( { args[0].as< account_name_type >(), args[1].as< string >() } ).votes;
}

DEFINE_API( condenser_api, get_account_votes )
{
   CHECK_ARG_SIZE( 1 )
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_votes( args );
   });
}

DEFINE_API( condenser_api, get_content )
{
   CHECK_ARG_SIZE( 2 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussion( { args[0].as< account_name_type >(), args[1].as< string >() } );
}

DEFINE_API( condenser_api, get_content_replies )
{
   CHECK_ARG_SIZE( 2 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_content_replies( { args[0].as< account_name_type >(), args[1].as< string >() } ).discussions;
}

DEFINE_API( condenser_api, get_tags_used_by_author )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_tags_used_by_author( { args[0].as< account_name_type >() } ).tags;
}

DEFINE_API( condenser_api, get_post_discussions_by_payout )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_post_discussions_by_payout(
      args[0].as< tags::get_post_discussions_by_payout_args >() ).discussions;
}

DEFINE_API( condenser_api, get_comment_discussions_by_payout )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_comment_discussions_by_payout(
      args[0].as< tags::get_comment_discussions_by_payout_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_trending )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_trending(
      args[0].as< tags::get_discussions_by_trending_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_created )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_created(
      args[0].as< tags::get_discussions_by_created_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_active )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_active(
      args[0].as< tags::get_discussions_by_active_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_cashout )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_cashout(
      args[0].as< tags::get_discussions_by_cashout_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_votes )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_votes(
      args[0].as< tags::get_discussions_by_votes_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_children )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_children(
      args[0].as< tags::get_discussions_by_children_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_hot )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_hot(
      args[0].as< tags::get_discussions_by_hot_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_feed )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_feed(
      args[0].as< tags::get_discussions_by_feed_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_blog )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_blog(
      args[0].as< tags::get_discussions_by_blog_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_comments )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_comments(
      args[0].as< tags::get_discussions_by_comments_args >() ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_promoted )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_promoted(
      args[0].as< tags::get_discussions_by_promoted_args >() ).discussions;
}

DEFINE_API( condenser_api, get_replies_by_last_update )
{
   CHECK_ARG_SIZE( 3 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_replies_by_last_update( { args[0].as< account_name_type >(), args[1].as< string >(), args[2].as< uint32_t >() } ).discussions;
}

DEFINE_API( condenser_api, get_discussions_by_author_before_date )
{
   CHECK_ARG_SIZE( 4 )
   FC_ASSERT( my->_tags_api, "tags_api_plugin not enabled." );

   return my->_tags_api->get_discussions_by_author_before_date( { args[0].as< account_name_type >(), args[1].as< string >(), args[2].as< time_point_sec >(), args[3].as< uint32_t >() } ).discussions;
}

DEFINE_API( condenser_api, get_account_history )
{
   CHECK_ARG_SIZE( 3 )
   FC_ASSERT( my->_account_history_api, "account_history_api_plugin not enabled." );

   return my->_account_history_api->get_account_history( { args[0].as< account_name_type >(), args[1].as< uint64_t >(), args[2].as< uint32_t >() } ).history;
}

DEFINE_API( condenser_api, broadcast_transaction )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_network_broadcast_api, "network_broadcast_api_plugin not enabled." );

   return my->_network_broadcast_api->broadcast_transaction( { args[0].as< signed_transaction >() } );
}

DEFINE_API( condenser_api, broadcast_transaction_synchronous )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_network_broadcast_api, "network_broadcast_api_plugin not enabled." );

   return my->_network_broadcast_api->broadcast_transaction_synchronous( { args[0].as< signed_transaction >() } );
}

DEFINE_API( condenser_api, broadcast_block )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_network_broadcast_api, "network_broadcast_api_plugin not enabled." );

   return my->_network_broadcast_api->broadcast_block( { args[0].as< signed_block >() } );
}

DEFINE_API( condenser_api, get_followers )
{
   CHECK_ARG_SIZE( 4 )
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_followers( { args[0].as< account_name_type >(), args[1].as< account_name_type >(), args[2].as< follow::follow_type >(), args[3].as< uint32_t >() } ).followers;
}

DEFINE_API( condenser_api, get_following )
{
   CHECK_ARG_SIZE( 4 )
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_following( { args[0].as< account_name_type >(), args[1].as< account_name_type >(), args[2].as< follow::follow_type >(), args[3].as< uint32_t >() } ).following;
}

DEFINE_API( condenser_api, get_follow_count )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_follow_count( { args[0].as< account_name_type >() } );
}

DEFINE_API( condenser_api, get_feed_entries )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_feed_entries( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).feed;
}

DEFINE_API( condenser_api, get_feed )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_feed( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).feed;
}

DEFINE_API( condenser_api, get_blog_entries )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_blog_entries( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).blog;
}

DEFINE_API( condenser_api, get_blog )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_blog( { args[0].as< account_name_type >(), args[1].as< uint32_t >(), args.size() == 3 ? args[2].as< uint32_t >() : 500 } ).blog;
}

DEFINE_API( condenser_api, get_account_reputations )
{
   FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_account_reputations( { args[0].as< account_name_type >(), args.size() == 2 ? args[1].as< uint32_t >() : 1000 } ).reputations;
}

DEFINE_API( condenser_api, get_reblogged_by )
{
   CHECK_ARG_SIZE( 2 )
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_reblogged_by( { args[0].as< account_name_type >(), args[1].as< string >() } ).accounts;
}

DEFINE_API( condenser_api, get_blog_authors )
{
   CHECK_ARG_SIZE( 1 )
   FC_ASSERT( my->_follow_api, "follow_api_plugin not enabled." );

   return my->_follow_api->get_blog_authors( { args[0].as< account_name_type >() } ).blog_authors;
}

DEFINE_API( condenser_api, get_ticker )
{
   CHECK_ARG_SIZE( 0 )
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_ticker( {} );
}

DEFINE_API( condenser_api, get_volume )
{
   CHECK_ARG_SIZE( 0 )
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_volume( {} );
}

DEFINE_API( condenser_api, get_order_book )
{
   FC_ASSERT( args.size() == 0 || args.size() == 1, "Expected 0-1 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_order_book( { args.size() == 1 ? args[0].as< uint32_t >() : 500 } );
}

DEFINE_API( condenser_api, get_trade_history )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_trade_history( { args[0].as< time_point_sec >(), args[1].as< time_point_sec >(), args.size() == 3 ? args[2].as< uint32_t >() : 1000 } ).trades;
}

DEFINE_API( condenser_api, get_recent_trades )
{
   FC_ASSERT( args.size() == 0 || args.size() == 1, "Expected 0-1 arguments, was ${n}", ("n", args.size()) );
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_recent_trades( { args.size() == 1 ? args[0].as< uint32_t >() : 1000 } ).trades;
}

DEFINE_API( condenser_api, get_market_history )
{
   CHECK_ARG_SIZE( 3 )
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_market_history( { args[0].as< uint32_t >(), args[1].as< time_point_sec >(), args[2].as< time_point_sec >() } ).buckets;
}

DEFINE_API( condenser_api, get_market_history_buckets )
{
   CHECK_ARG_SIZE( 0 )
   FC_ASSERT( my->_market_history_api, "market_history_api_plugin not enabled." );

   return my->_market_history_api->get_market_history_buckets( {} ).bucket_sizes;
}

} } } // steem::plugins::condenser_api
