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

      follow_plugin&                                                       _self;
      std::shared_ptr< json_evaluator_registry< steemit::follow::follow_plugin_operation > >  _evaluator_registry;
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

struct operation_visitor
{
   follow_plugin& _plugin;

   operation_visitor( follow_plugin& plugin )
      :_plugin( plugin ) {}

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
      FC_LOG_AND_RETHROW()
   }

   void operator()( const comment_operation& op )const
   {
      try
      {
         if( op.parent_author.size() > 0 ) return;
         auto& db = _plugin.database();
         const auto& c = db.get_comment( op.author, op.permlink );

         const auto& idx = db.get_index_type< follow_index >().indices().get< by_following_follower >();
         auto itr = idx.find( op.author ); //boost::make_tuple( op.author, op.following ) );

         while( itr != idx.end() && itr->following == op.author )
         {
            db.create< feed_object >( [&]( feed_object& f )
            {
                f.account = db.get_account( itr->following ).id;
                f.comment = c.id;
            });

            ++itr;
         }
      }
      FC_LOG_AND_RETHROW()
   }
};

void follow_plugin_impl::on_operation( const operation_object& op_obj )
{
   try
   {
      op_obj.op.visit( operation_visitor( _self ) );
   }
   catch( fc::assert_exception )
   {
      if( database().is_producing() ) throw;
   }
}

} // end namespace detail

follow_plugin::follow_plugin( application* app )
   : plugin( app ), my( new detail::follow_plugin_impl( *this ) ) {}

void follow_api::on_api_startup() {}

void follow_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   ilog("Intializing follow plugin" );
   database().post_apply_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< follow_index  > >();
   database().add_index< primary_index< feed_index  > >();

   app().register_api_factory<follow_api>("follow_api");
}

} } // steemit::follow

STEEMIT_DEFINE_PLUGIN( follow, steemit::follow::follow_plugin )

//DEFINE_OPERATION_TYPE( steemit::follow::follow_plugin_operation )
