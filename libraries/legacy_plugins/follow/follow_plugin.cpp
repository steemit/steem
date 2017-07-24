#include <steemit/follow/follow_api.hpp>
#include <steemit/follow/follow_objects.hpp>
#include <steemit/follow/follow_operations.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/protocol/config.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/generic_custom_operation_interpreter.hpp>
#include <steemit/chain/operation_notification.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>

#include <graphene/schema/schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <memory>

namespace steemit { namespace follow {

namespace detail
{

using namespace steemit::protocol;

class follow_plugin_impl
{
   public:
      follow_plugin_impl( follow_plugin& _plugin ) : _self( _plugin ) {}

      void plugin_initialize();

      steemit::chain::database& database()
      {
         return _self.database();
      }

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );

      follow_plugin&                                                                         _self;
      std::shared_ptr< generic_custom_operation_interpreter< steemit::follow::follow_plugin_operation > > _custom_operation_interpreter;
};

void follow_plugin_impl::plugin_initialize()
{
   // Each plugin needs its own evaluator registry.
   _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< steemit::follow::follow_plugin_operation > >( database() );

   // Add each operation evaluator to the registry
   _custom_operation_interpreter->register_evaluator<follow_evaluator>( &_self );
   _custom_operation_interpreter->register_evaluator<reblog_evaluator>( &_self );

   // Add the registry to the database so the database can delegate custom ops to the plugin
   database().set_custom_operation_interpreter( _self.plugin_name(), _custom_operation_interpreter );
}

struct pre_operation_visitor
{
   follow_plugin& _plugin;

   pre_operation_visitor( follow_plugin& plugin )
      : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin.database();
         const auto& c = db.get_comment( op.author, op.permlink );

         if( db.calculate_discussion_payout_time( c ) == fc::time_point_sec::maximum() ) return;

         const auto& cv_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( std::make_tuple( c.id, db.get_account( op.voter ).id ) );

         if( cv != cv_idx.end() )
         {
            const auto& rep_idx = db.get_index< reputation_index >().indices().get< by_account >();
            auto rep = rep_idx.find( op.author );

            if( rep != rep_idx.end() )
            {
               db.modify( *rep, [&]( reputation_object& r )
               {
                  r.reputation -= ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
               });
            }
         }
      }
      catch( const fc::exception& e ) {}
   }

   void operator()( const delete_comment_operation& op )const
   {
      try
      {
         auto& db = _plugin.database();
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
   follow_plugin& _plugin;

   post_operation_visitor( follow_plugin& plugin )
      : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const custom_json_operation& op )const
   {
      try
      {
         if( op.id == FOLLOW_PLUGIN_NAME )
         {
            custom_json_operation new_cop;

            new_cop.required_auths = op.required_auths;
            new_cop.required_posting_auths = op.required_posting_auths;
            new_cop.id = _plugin.plugin_name();
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
            std::shared_ptr< custom_operation_interpreter > eval = _plugin.database().get_custom_json_evaluator( op.id );
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
         auto& db = _plugin.database();
         const auto& c = db.get_comment( op.author, op.permlink );

         if( c.created != db.head_block_time() ) return;

         const auto& idx = db.get_index< follow_index >().indices().get< by_following_follower >();
         const auto& comment_idx = db.get_index< feed_index >().indices().get< by_comment >();
         auto itr = idx.find( op.author );

         const auto& feed_idx = db.get_index< feed_index >().indices().get< by_feed >();

         if( db.head_block_time() >= _plugin.start_feeds )
         {
            while( itr != idx.end() && itr->following == op.author )
            {
               if( itr->what & ( 1 << blog ) )
               {
                  uint32_t next_id = 0;
                  auto last_feed = feed_idx.lower_bound( itr->follower );

                  if( last_feed != feed_idx.end() && last_feed->account == itr->follower )
                  {
                     next_id = last_feed->account_feed_id + 1;
                  }

                  if( comment_idx.find( boost::make_tuple( c.id, itr->follower ) ) == comment_idx.end() )
                  {
                     db.create< feed_object >( [&]( feed_object& f )
                     {
                        f.account = itr->follower;
                        f.comment = c.id;
                        f.account_feed_id = next_id;
                     });

                     const auto& old_feed_idx = db.get_index< feed_index >().indices().get< by_old_feed >();
                     auto old_feed = old_feed_idx.lower_bound( itr->follower );

                     while( old_feed->account == itr->follower && next_id - old_feed->account_feed_id > _plugin.max_feed_size )
                     {
                        db.remove( *old_feed );
                        old_feed = old_feed_idx.lower_bound( itr->follower );
                     }
                  }
               }

               ++itr;
            }
         }

         const auto& blog_idx = db.get_index< blog_index >().indices().get< by_blog >();
         const auto& comment_blog_idx = db.get_index< blog_index >().indices().get< by_comment >();
         auto last_blog = blog_idx.lower_bound( op.author );
         uint32_t next_id = 0;

         if( last_blog != blog_idx.end() && last_blog->account == op.author )
         {
            next_id = last_blog->blog_feed_id + 1;
         }

         if( comment_blog_idx.find( boost::make_tuple( c.id, op.author ) ) == comment_blog_idx.end() )
         {
            db.create< blog_object >( [&]( blog_object& b)
            {
               b.account = op.author;
               b.comment = c.id;
               b.blog_feed_id = next_id;
            });

            const auto& old_blog_idx = db.get_index< blog_index >().indices().get< by_old_blog >();
            auto old_blog = old_blog_idx.lower_bound( op.author );

            while( old_blog->account == op.author && next_id - old_blog->blog_feed_id > _plugin.max_feed_size )
            {
               db.remove( *old_blog );
               old_blog = old_blog_idx.lower_bound( op.author );
            }
         }
      }
      FC_LOG_AND_RETHROW()
   }

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin.database();
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
      note.op.visit( pre_operation_visitor( _self ) );
   }
   catch( const fc::assert_exception& )
   {
      if( database().is_producing() ) throw;
   }
}

void follow_plugin_impl::post_operation( const operation_notification& note )
{
   try
   {
      note.op.visit( post_operation_visitor( _self ) );
   }
   catch( fc::assert_exception )
   {
      if( database().is_producing() ) throw;
   }
}

} // end namespace detail

follow_plugin::follow_plugin( application* app )
   : plugin( app ), my( new detail::follow_plugin_impl( *this ) ) {}

void follow_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
      ("follow-max-feed-size", boost::program_options::value< uint32_t >()->default_value( 500 ), "Set the maximum size of cached feed for an account" )
      ("follow-start-feeds", boost::program_options::value< uint32_t >()->default_value( 0 ), "Block time (in epoch seconds) when to start calculating feeds" )
      ;
   cfg.add( cli );
}

void follow_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing follow plugin" );
      chain::database& db = database();
      my->plugin_initialize();

      db.pre_apply_operation.connect( [&]( const operation_notification& o ){ my->pre_operation( o ); } );
      db.post_apply_operation.connect( [&]( const operation_notification& o ){ my->post_operation( o ); } );
      add_plugin_index< follow_index       >(db);
      add_plugin_index< feed_index         >(db);
      add_plugin_index< blog_index         >(db);
      add_plugin_index< reputation_index   >(db);
      add_plugin_index< follow_count_index >(db);
      add_plugin_index< blog_author_stats_index >(db);

      if( options.count( "follow-max-feed-size" ) )
      {
         uint32_t feed_size = options[ "follow-max-feed-size" ].as< uint32_t >();
         max_feed_size = feed_size;
      }

      if( options.count( "follow-start-feeds" ) )
      {
         start_feeds = fc::time_point_sec( options[ "follow-start-feeds" ].as< uint32_t >() );
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

void follow_plugin::plugin_startup()
{
   app().register_api_factory<follow_api>("follow_api");
}

} } // steemit::follow

STEEMIT_DEFINE_PLUGIN( follow, steemit::follow::follow_plugin )

//DEFINE_OPERATION_TYPE( steemit::follow::follow_plugin_operation )
