
#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/follow/follow_plugin.hpp>
#include <steem/plugins/follow/follow_objects.hpp>
#include <steem/plugins/follow/follow_operations.hpp>
#include <steem/plugins/follow/inc_performance.hpp>

#include <steem/chain/util/impacted.hpp>

#include <steem/protocol/config.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <memory>

namespace steem { namespace plugins { namespace follow {

using namespace steem::protocol;

namespace detail {

class follow_plugin_impl
{
   public:
      follow_plugin_impl( follow_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}
      ~follow_plugin_impl() {}

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );

      chain::database&              _db;
      follow_plugin&                _self;
      boost::signals2::connection   _pre_apply_operation_conn;
      boost::signals2::connection   _post_apply_operation_conn;
};

struct pre_operation_visitor
{
   follow_plugin_impl& _plugin;

   pre_operation_visitor( follow_plugin_impl& plugin )
      : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin._db;
         const auto& c = db.get_comment( op.author, op.permlink );

         if( db.calculate_discussion_payout_time( c ) == fc::time_point_sec::maximum() ) return;

         const auto& cv_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( boost::make_tuple( c.id, db.get_account( op.voter ).id ) );

         if( cv != cv_idx.end() )
         {
            auto rep_delta = ( cv->rshares >> 6 );

            const auto& rep_idx = db.get_index< reputation_index >().indices().get< by_account >();
            auto voter_rep = rep_idx.find( op.voter );
            auto author_rep = rep_idx.find( op.author );

            if( author_rep != rep_idx.end() )
            {
               // Rule #1: Must have non-negative reputation to effect another user's reputation
               if( voter_rep != rep_idx.end() && voter_rep->reputation < 0 ) return;

               // Rule #2: If you are down voting another user, you must have more reputation than them to impact their reputation
               if( cv->rshares < 0 && !( voter_rep != rep_idx.end() && voter_rep->reputation > author_rep->reputation - rep_delta ) ) return;

               if( rep_delta == author_rep->reputation )
               {
                  db.remove( *author_rep );
               }
               else
               {
                  db.modify( *author_rep, [&]( reputation_object& r )
                  {
                     r.reputation -= ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
                  });
               }
            }
         }
      }
      catch( const fc::exception& e ) {}
   }

   void operator()( const delete_comment_operation& op )const
   {
      try
      {
         auto& db = _plugin._db;
         const auto* comment = db.find_comment( op.author, op.permlink );

         if( comment == nullptr ) return;
         if( comment->parent_author.size() ) return;

         const auto& feed_idx = db.get_index< feed_index >().indices().get< by_comment >();
         auto itr = feed_idx.lower_bound( comment->id );

         while( itr != feed_idx.end() && itr->comment == comment->id )
         {
            const auto& old_feed = *itr;
            ++itr;
            db.remove( old_feed );
         }

         const auto& blog_idx = db.get_index< blog_index >().indices().get< by_comment >();
         auto blog_itr = blog_idx.lower_bound( comment->id );

         while( blog_itr != blog_idx.end() && blog_itr->comment == comment->id )
         {
            const auto& old_blog = *blog_itr;
            ++blog_itr;
            db.remove( old_blog );
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }
};

struct post_operation_visitor
{
   follow_plugin_impl& _plugin;

   performance perf;

   post_operation_visitor( follow_plugin_impl& plugin )
      : _plugin( plugin ), perf( plugin._db ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const custom_json_operation& op )const
   {
      try
      {
         if( op.id == STEEM_FOLLOW_PLUGIN_NAME )
         {
            custom_json_operation new_cop;

            new_cop.required_auths = op.required_auths;
            new_cop.required_posting_auths = op.required_posting_auths;
            new_cop.id = _plugin._self.name();
            follow_operation fop;

            try
            {
               fop = fc::json::from_string( op.json ).as< follow_operation >();
            }
            catch( const fc::exception& )
            {
               return;
            }

            auto new_fop = follow_plugin_operation( fop );
            new_cop.json = fc::json::to_string( new_fop );
            std::shared_ptr< custom_operation_interpreter > eval = _plugin._db.get_custom_json_evaluator( op.id );
            eval->apply( new_cop );
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }

   void operator()( const comment_operation& op )const
   {
      try
      {
         if( op.parent_author.size() > 0 ) return;
         auto& db = _plugin._db;
         const auto& c = db.get_comment( op.author, op.permlink );

         if( c.created != db.head_block_time() ) return;

         const auto& idx = db.get_index< follow_index >().indices().get< by_following_follower >();
         const auto& comment_idx = db.get_index< feed_index >().indices().get< by_comment >();
         const auto& old_feed_idx = db.get_index< feed_index >().indices().get< by_feed >();
         auto itr = idx.find( op.author );

         performance_data pd;

         if( db.head_block_time() >= _plugin._self.start_feeds )
         {
            while( itr != idx.end() && itr->following == op.author )
            {
               if( itr->what & ( 1 << blog ) )
               {
                  auto feed_itr = comment_idx.find( boost::make_tuple( c.id, itr->follower ) );
                  bool is_empty = feed_itr == comment_idx.end();

                  pd.init( c.id, is_empty );
                  uint32_t next_id = perf.delete_old_objects< performance_data::t_creation_type::part_feed >( old_feed_idx, itr->follower, _plugin._self.max_feed_size, pd );

                  if( pd.s.creation && is_empty )
                  {
                     db.create< feed_object >( [&]( feed_object& f )
                     {
                        f.account = itr->follower;
                        f.comment = c.id;
                        f.account_feed_id = next_id;
                     });
                  }

               }
               ++itr;
            }
         }

         const auto& comment_blog_idx = db.get_index< blog_index >().indices().get< by_comment >();
         auto blog_itr = comment_blog_idx.find( boost::make_tuple( c.id, op.author ) );
         const auto& old_blog_idx = db.get_index< blog_index >().indices().get< by_blog >();
         bool is_empty = blog_itr == comment_blog_idx.end();

         pd.init( c.id, is_empty );
         uint32_t next_id = perf.delete_old_objects< performance_data::t_creation_type::full_blog >( old_blog_idx, op.author, _plugin._self.max_feed_size, pd );

         if( pd.s.creation && is_empty )
         {
            db.create< blog_object >( [&]( blog_object& b)
            {
               b.account = op.author;
               b.comment = c.id;
               b.blog_feed_id = next_id;
            });
         }
      }
      FC_LOG_AND_RETHROW()
   }

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin._db;
         const auto& comment = db.get_comment( op.author, op.permlink );

         if( db.calculate_discussion_payout_time( comment ) == fc::time_point_sec::maximum() )
            return;

         const auto& cv_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( boost::make_tuple( comment.id, db.get_account( op.voter ).id ) );

         const auto& rep_idx = db.get_index< reputation_index >().indices().get< by_account >();
         auto voter_rep = rep_idx.find( op.voter );
         auto author_rep = rep_idx.find( op.author );

         // Rules are a plugin, do not effect consensus, and are subject to change.
         // Rule #1: Must have non-negative reputation to effect another user's reputation
         if( voter_rep != rep_idx.end() && voter_rep->reputation < 0 ) return;

         if( author_rep == rep_idx.end() )
         {
            // Rule #2: If you are down voting another user, you must have more reputation than them to impact their reputation
            // User rep is 0, so requires voter having positive rep
            if( cv->rshares < 0 && !( voter_rep != rep_idx.end() && voter_rep->reputation > 0 )) return;

            db.create< reputation_object >( [&]( reputation_object& r )
            {
               r.account = op.author;
               r.reputation = ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
            });
         }
         else
         {
            // Rule #2: If you are down voting another user, you must have more reputation than them to impact their reputation
            if( cv->rshares < 0 && !( voter_rep != rep_idx.end() && voter_rep->reputation > author_rep->reputation ) ) return;

            db.modify( *author_rep, [&]( reputation_object& r )
            {
               r.reputation += ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
            });
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }
};

void follow_plugin_impl::pre_operation( const operation_notification& note )
{
   try
   {
      note.op.visit( pre_operation_visitor( *this ) );
   }
   catch( const fc::assert_exception& )
   {
      if( _db.is_producing() ) throw;
   }
}

void follow_plugin_impl::post_operation( const operation_notification& note )
{
   try
   {
      note.op.visit( post_operation_visitor( *this ) );
   }
   catch( fc::assert_exception )
   {
      if( _db.is_producing() ) throw;
   }
}

} // detail

follow_plugin::follow_plugin() {}

follow_plugin::~follow_plugin() {}

void follow_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cfg.add_options()
      ("follow-max-feed-size", boost::program_options::value< uint32_t >()->default_value( 500 ), "Set the maximum size of cached feed for an account" )
      ("follow-start-feeds", boost::program_options::value< uint32_t >()->default_value( 0 ), "Block time (in epoch seconds) when to start calculating feeds" )
      ;
}

void follow_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing follow plugin" );

      my = std::make_unique< detail::follow_plugin_impl >( *this );

      // Each plugin needs its own evaluator registry.
      _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< steem::plugins::follow::follow_plugin_operation > >( my->_db, name() );

      // Add each operation evaluator to the registry
      _custom_operation_interpreter->register_evaluator< follow_evaluator >( this );
      _custom_operation_interpreter->register_evaluator< reblog_evaluator >( this );

      // Add the registry to the database so the database can delegate custom ops to the plugin
      my->_db.register_custom_operation_interpreter( _custom_operation_interpreter );

      my->_pre_apply_operation_conn = my->_db.add_pre_apply_operation_handler( [&]( const operation_notification& note ){ my->pre_operation( note ); }, *this, 0 );
      my->_post_apply_operation_conn = my->_db.add_post_apply_operation_handler( [&]( const operation_notification& note ){ my->post_operation( note ); }, *this, 0 );
      add_plugin_index< follow_index            >( my->_db );
      add_plugin_index< feed_index              >( my->_db );
      add_plugin_index< blog_index              >( my->_db );
      add_plugin_index< reputation_index        >( my->_db );
      add_plugin_index< follow_count_index      >( my->_db );
      add_plugin_index< blog_author_stats_index >( my->_db );

      fc::mutable_variant_object state_opts;

      if( options.count( "follow-max-feed-size" ) )
      {
         uint32_t feed_size = options[ "follow-max-feed-size" ].as< uint32_t >();
         max_feed_size = feed_size;
         state_opts[ "follow-max-feed-size" ] = feed_size;
      }

      if( options.count( "follow-start-feeds" ) )
      {
         start_feeds = fc::time_point_sec( options[ "follow-start-feeds" ].as< uint32_t >() );
         state_opts[ "follow-start-feeds" ] = start_feeds;
      }

      appbase::app().get_plugin< chain::chain_plugin >().report_state_options( name(), state_opts );
   }
   FC_CAPTURE_AND_RETHROW()
}

void follow_plugin::plugin_startup() {}

void follow_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_pre_apply_operation_conn );
   chain::util::disconnect_signal( my->_post_apply_operation_conn );
}

} } } // steem::plugins::follow
