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

      follow_plugin&                                                 _self;
      evaluator_registry< steemit::follow::follow_plugin_operation > _evaluator_registry;
};

follow_plugin_impl::follow_plugin_impl( follow_plugin& _plugin )
   : _self( _plugin ), _evaluator_registry( _plugin.database() )
{
   _evaluator_registry.register_evaluator< follow_evaluator >( &_self );
}

void follow_plugin_impl::on_operation( const operation_object& op_obj ) {
   steemit::chain::database& db = database();

   try {
      if( op_obj.op.which() == operation::tag< custom_json_operation >::value ) {
         const custom_json_operation& cop = op_obj.op.get< custom_json_operation >();
        // idump(("json op")(cop));
         if( cop.id == "follow" )
         {
            custom_json_operation new_cop;

            new_cop.required_auths = cop.required_auths;
            new_cop.required_posting_auths = cop.required_posting_auths;
            new_cop.id = cop.id;
            follow_plugin_operation new_op;

            try
            {
               auto op = fc::json::from_string( cop.json ).as< follow_operation >();
               new_op = op;
            }
            catch( fc::assert_exception )
            {
               return;
            }

            new_cop.json = fc::json::to_string( new_op );
            std::shared_ptr< generic_json_evaluator_registry > eval = db.get_custom_json_evaluator( cop.id );
            eval->apply( new_cop );
         }
      }
      else if ( op_obj.op.which() == operation::tag<comment_operation>::value )
      {
         const auto& op = op_obj.op.get<comment_operation>();
         const auto& c = db.get_comment( op.author, op.permlink );

         const auto& idx = db.get_index_type<follow_index>().indices().get<by_following_follower>();
         auto itr = idx.find( op.author ); //boost::make_tuple( op.author, op.following ) );

         while( itr != idx.end() && itr->following == op.author )
         {
            db.create<feed_object>( [&]( feed_object& f )
            {
                f.account = db.get_account( itr->following ).id;
                f.comment = c.id;
            });

            ++itr;
         }
      }
   }
   catch ( const fc::exception& )
   {
      if( db.is_producing() ) throw;
   }
}

} // end namespace detail

follow_plugin::follow_plugin( application* app ) : plugin( app ), my( new detail::follow_plugin_impl(*this) ){}

void follow_api::on_api_startup() {}

void follow_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   ilog("Intializing follow plugin" );
   database().post_apply_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< follow_index  > >();
   database().add_index< primary_index< feed_index  > >();

   app().register_api_factory<follow_api>("follow_api");
}

} }

STEEMIT_DEFINE_PLUGIN( follow, steemit::follow::follow_plugin )

DEFINE_OPERATION_TYPE( steemit::follow::follow_plugin_operation )
