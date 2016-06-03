#include <steemit/follow/follow_plugin.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/chain/config.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace steemit { namespace follow {

namespace detail
{

class follow_plugin_impl
{
   public:
      follow_plugin_impl(follow_plugin& _plugin)
      : _self( _plugin ) { }

      steemit::chain::database& database() { return _self.database(); }

      void on_operation( const operation_object& op_obj );

      follow_plugin& _self;
};


void follow_plugin_impl::on_operation( const operation_object& op_obj ) {
   steemit::chain::database& db = database();

   try {
      if( op_obj.op.which() == operation::tag<custom_json_operation>::value ) {
         const custom_json_operation& cop = op_obj.op.get<custom_json_operation>();
         idump(("json op")(cop));
         if( cop.id == "follow" )  {
            auto op = fc::json::from_string(cop.json).as<follow_operation>();
            FC_ASSERT( cop.required_auths.find( op.follower ) != cop.required_auths.end() ||
                       cop.required_posting_auths.find( op.follower ) != cop.required_posting_auths.end() 
                       , "follower didn't sign message" );

            FC_ASSERT( op.follower != op.following );


            const auto& idx = db.get_index_type<follow_index>().indices().get<by_follower_following>();
            auto itr = idx.find( boost::make_tuple( op.follower, op.following ) );
            if( itr == idx.end() ) {
               db.create<follow_object>( [&]( follow_object& obj ) {
                  obj.follower = op.follower;
                  obj.following = op.following;
                  obj.what = op.what;
               });
            } else {
               db.modify( *itr, [&]( follow_object& obj ) {
                  obj.what = op.what;
               });
            }

         }
      }
   } catch ( const fc::exception& ) {
      if( db.is_producing() ) throw;
   }
}

} // end namespace detail

follow_plugin::follow_plugin() : my( new detail::follow_plugin_impl(*this) ){}

std::string follow_plugin::plugin_name()const
{
   return "follow";
}


void follow_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   ilog("Intializing follow plugin" );
   database().on_applied_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< follow_index  > >();

   app().register_api_factory<follow_api>("follow_api");
}

vector<follow_object> follow_api::get_followers( string following, string start_follower, uint16_t limit )const {
   FC_ASSERT( limit <= 100 );
   vector<follow_object> result;
   const auto& idx = _app->chain_database()->get_index_type<follow_index>().indices().get<by_following_follower>();
   auto itr = idx.lower_bound( std::make_tuple( following, start_follower ) );
   while( itr != idx.end() && limit && itr->following == following ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }

   return result;
}

vector<follow_object> follow_api::get_following( string follower, string start_following, uint16_t limit )const {
   FC_ASSERT( limit <= 100 );
   vector<follow_object> result;
   const auto& idx = _app->chain_database()->get_index_type<follow_index>().indices().get<by_follower_following>();
   auto itr = idx.lower_bound( std::make_tuple( follower, start_following ) );
   while( itr != idx.end() && limit && itr->follower == follower ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }

   return result;
}


} }

STEEMIT_DEFINE_PLUGIN( follow, steemit::follow::follow_plugin )
