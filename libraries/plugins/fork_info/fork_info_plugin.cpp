#include <steemit/fork_info/fork_info_plugin.hpp>

#include <steemit/protocol/types.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/app/api.hpp>
#include <steemit/app/database_api.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/api.hpp>
#include <fc/smart_ref_impl.hpp>

namespace steemit { namespace fork_info {

namespace detail {
   using namespace steemit::protocol;

   struct fork_info_plugin_impl
   {
      public:
         fork_info_plugin_impl( fork_info_plugin& plugin )
            :_self( plugin ) {}
         virtual ~fork_info_plugin_impl() {}

         void on_block( const signed_block& b );

         fork_info_plugin&                                 _self;
         std::map< uint32_t, std::vector< signed_block > > _last_blocks;
   };

   void fork_info_plugin_impl::on_block( const signed_block& b )
   {
      auto& db = _self.database();
      auto lib = db.get_dynamic_global_properties().last_irreversible_block_num;
      
      if( b.block_num() > lib ) // Should not be needed, but just in case
         _last_blocks[ b.block_num() ].push_back( b );

      if( _last_blocks.find( lib ) != _last_blocks.end() ) 
      {
         auto& blocks = _last_blocks[ lib ];
         fc::optional< signed_block > included_block = db.fetch_block_by_number( lib );
         std::set< signature_type > printed_blocks;

         for( auto& orphan : blocks )
         {
            if( orphan.witness_signature != included_block->witness_signature
               && printed_blocks.find( orphan.witness_signature ) != printed_blocks.end() )
            {
               ilog( "Orphaned Block --- Num:${n} Data:${d}", ("n", lib )("d", orphan) );
               printed_blocks.insert( orphan.witness_signature );  
            }
         }

         _last_blocks.erase( lib );
      }
   }
} // detail

fork_info_plugin::fork_info_plugin( application* app )
   : plugin( app ), _my( new detail::fork_info_plugin_impl( *this ) ) {}

fork_info_plugin::~fork_info_plugin() {}

void fork_info_plugin::plugin_set_program_options(
      boost::program_options::options_description& cli,
      boost::program_options::options_description& cfg ) {}

void fork_info_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      database().applied_block.connect( [&]( const chain::signed_block& b ){ _my->on_block( b ); } );
   } FC_CAPTURE_AND_RETHROW()
}

void fork_info_plugin::plugin_startup() {}

} } // steemit::fork_info

STEEMIT_DEFINE_PLUGIN( fork_info, steemit::fork_info::fork_info_plugin )
