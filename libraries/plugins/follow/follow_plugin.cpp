#include <steemit/follow/follow_api.hpp>
#include <steemit/follow/follow_objects.hpp>
#include <steemit/follow/follow_operations.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/chain/config.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/json_evaluator_registry.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <memory>

namespace steemit { namespace follow {

namespace detail
{

class follow_plugin_impl
{
   public:
      follow_plugin_impl( follow_plugin& _plugin );

      steemit::chain::database& database()
      {
         return _self.database();
      }

      void on_operation( const operation_object& op_obj );
      void pre_operation( const operation_object& op_0bj );

      follow_plugin&                                                                         _self;
      std::shared_ptr< json_evaluator_registry< steemit::follow::follow_plugin_operation > > _evaluator_registry;
};

follow_plugin_impl::follow_plugin_impl( follow_plugin& _plugin )
   : _self( _plugin )
{
   // Each plugin needs its own evaluator registry.
   _evaluator_registry = std::make_shared< json_evaluator_registry< steemit::follow::follow_plugin_operation > >( database() );

   // Add each operation evaluator to the registry
   _evaluator_registry->register_evaluator<follow_evaluator>( &_self );
   _evaluator_registry->register_evaluator<reblog_evaluator>( &_self );

   // Add the registry to the database so the database can delegate custom ops to the plugin
   database().set_custom_json_evaluator( _self.plugin_name(), _evaluator_registry );
}

struct pre_operation_visitor
{
   follow_plugin& _plugin;

   pre_operation_visitor( follow_plugin& plugin )
      : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >___POSIX_C_DEPRECATED_STARTING_198808L
   void operator()( const T& )const {}

   void operator()( const vote_operation& op )
   {
      try
      {
         auto& db = _plugin.database();
         const auto& c = db.get_comment( op.author, op.permlink );

         if( c.mode != first_payout ) return;

         const auto& a = db.get_account( op.voter );

         const auto& cv_idx = db.get_index_type< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( boost::make_tuple( c.id, a.id ) );

         if( cv != cv_idx.end() )
         {
            const auto& rep_idx = db.get_index_type< reputation_index >().indices().get< by_account >();
            auto rep = rep_idx.find( cv->voter );

            db.modify( *rep, [&]( reputation_object& r )
            {
               r.reputation -= cv->rshares;
            });
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }
};

struct on_operation_visitor
{
   follow_plugin& _plugin;

   on_operation_visitor( follow_plugin& plugin )
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
            catch( fc::assert_exception )
            {
               return;
            }

            auto new_fop = follow_plugin_operation( fop );
            new_cop.json = fc::json::to_string( new_fop );
            std::shared_ptr< generic_json_evaluator_registry > eval = _plugin.database().get_custom_json_evaluator( op.id );
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

         const auto& idx = db.get_index_type< follow_index >().indices().get< by_following_follower >();
         auto itr = idx.find( op.author );

         const auto& feed_idx = db.get_index_type< feed_index >().indices().get< by_feed >();

         while( itr != idx.end() && itr->following == op.author )
         {
            if( itr->what.find( follow_type::blog ) != itr->what.end() )
            {
               auto account_id = db.get_account( itr->follower ).id;
               uint32_t next_id = 0;
               auto last_feed = feed_idx.lower_bound( account_id );

               if( last_feed != feed_idx.end() && last_feed->account == account_id )
               {
                  next_id = last_feed->account_feed_id + 1;
               }

               db.create< feed_object >( [&]( feed_object& f )
               {
                  f.account = account_id;
                  f.comment = c.id;
                  f.account_feed_id = next_id;
               });

               const auto& old_feed_idx = db.get_index_type< feed_index >().indices().get< by_old_feed >();
               auto old_feed = old_feed_idx.lower_bound( account_id );

               while( old_feed->account == account_id && next_id - old_feed->account_feed_id > _plugin.max_feed_size )
               {
                  db.remove( *old_feed );
                  old_feed = old_feed_idx.lower_bound( account_id );
               };
            }

            ++itr;
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin.database();
         const auto& comment = db.get_comment( op.author, op.permlink );

         if( comment.mode != first_payout )
            return;

         const auto& voter_id = db.get_account( op.voter ).id;
         const auto& author_id = db.get_account( op.author ).id;
         const auto& cv_idx = db.get_index_type< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( boost::make_tuple( comment.id, voter_id ) );

         const auto& rep_idx = db.get_index_type< reputation_index >().indices().get< by_account >();
         auto voter = rep_idx.find( voter_id );
         auto rep = rep_idx.find( author_id );

         if( rep == rep_idx.end() )
         {
            db.create< reputation_object >( [&]( reputation_object& r )
            {
               r.account = author_id;
               r.reputation = cv->rshares;
            });
         }
         else
         {
            if( cv->rshares < 0 )            {
               FC_ASSERT( voter != rep_idx.end() && voter->reputation > rep->reputation );

            db.modify( *rep, [&]( reputation_object& r )
            {
               r.reputation += cv->rshares;
            });
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }
};

void follow_plugin_impl::pre_operation( const operation_object& op_obj )
{
   try
   {
      op_obj.op.visit( pre_operation_visitor( _self ) );
   }
   catch( const fc::assert_exception& )
   {
      if( database().is_producing() ) throw;
   }
}

void follow_plugin_impl::on_operation( const operation_object& op_obj )
{
   try
   {
      op_obj.op.visit( on_operation_visitor( _self ) );
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
      ;
   cfg.add( cli );
}

void follow_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing follow plugin" );
      database().pre_apply_operation.connect( [&]( const operation_object& o ){ my->pre_operation( o ); } );
      database().post_apply_operation.connect( [&]( const operation_object& o ){ my->on_operation( o ); } );
      database().add_index< primary_index< follow_index > >();
      database().add_index< primary_index< feed_index > >();
      database().add_index< primary_index< reputation_index > >();

      if( options.count( "follow-max-feed-size" ) )
      {
         uint32_t feed_size = options[ "follow-max-feed-size" ].as< uint32_t >();
         max_feed_size = feed_size;
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
