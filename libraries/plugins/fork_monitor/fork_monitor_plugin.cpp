#include <steem/plugins/fork_monitor/fork_monitor_plugin.hpp>

#include <steem/protocol/types.hpp>

#include <steem/chain/database.hpp>

#include <fc/network/http/websocket.hpp>

#include <fc/smart_ref_impl.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/operation_notification.hpp>

#include <graphene/schema/schema.hpp>
#include <graphene/schema/schema_impl.hpp>

namespace steem { namespace plugins { namespace fork_monitor {

namespace detail {
   using namespace steem::protocol;

   class fork_monitor_plugin_impl
   {
      public:
         fork_monitor_plugin_impl() : db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {};
         ~fork_monitor_plugin_impl() {};

         void on_block( const signed_block& b );

         std::map< uint32_t, signed_block > block_map;
         chain::database& db;
   };

   void fork_monitor_plugin_impl::on_block( const signed_block& b )
   {
     auto lib = db.get_dynamic_global_properties().last_irreversible_block_num;
     if( b.block_num() > lib ) { // Should not be needed, but just in case
         /* Check for deviant block */
         signed_block old_block;
         map<uint32_t, signed_block>::iterator it = block_map.find(b.block_num());
         if(it != block_map.end()){
            old_block = it->second;
            if(old_block.witness_signature != b.witness_signature){
               ilog( "Orphaned Block ${n} --- Data:${d}", ("n", old_block.block_num() )("d", old_block) );
               block_map[ b.block_num() ] = b;
            }
         }
         else {
            block_map[ b.block_num() ] = b;
            if(block_map.size() > 21){
               it = block_map.find(lib);
               block_map.erase ( block_map.begin(), it );    // erasing up to lib
            }
         }
      }
   }
} // detail

fork_monitor_plugin::fork_monitor_plugin() : _my( new detail::fork_monitor_plugin_impl() ) {}
fork_monitor_plugin::~fork_monitor_plugin(){}

void fork_monitor_plugin::set_program_options(
      boost::program_options::options_description& cli,
      boost::program_options::options_description& cfg ) {}

void fork_monitor_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
      {
         chain::database& _db = appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
         _db.applied_block.connect( [&]( const chain::signed_block& b ){ _my->on_block( b ); } );
      }
   FC_CAPTURE_AND_RETHROW()
}

#ifdef IS_TEST_NET
std::map< uint32_t, signed_block > fork_monitor_plugin::get_map()
{
   return _my->block_map;
}
#endif

void fork_monitor_plugin::plugin_startup() {}

void fork_monitor_plugin::plugin_shutdown() {}
} } } // steemit::fork_monitor
