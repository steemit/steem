#include <steemit/fork_info/fork_info_plugin.hpp>

#include <steemit/protocol/types.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/app/api.hpp>
#include <steemit/app/database_api.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/api.hpp>
#include <fc/smart_ref_impl.hpp>

namespace steemit { namespace plugin { namespace fork_info {

namespace detail {
   using namespace steemit::protocol;

   class fork_info_plugin_impl
   {
      public:
         fork_info_plugin_impl( fork_info_plugin& plugin )
            :_self( plugin ) {}
         virtual ~fork_info_plugin_impl() {}

         void on_block( const signed_block& b );

         fork_info_plugin&       _self;
         std::map<int,std::vector<signed_block>> _last_blocks;
         std::map<int,std::vector<signed_block>>::iterator it;
   };

   void fork_info_plugin_impl::on_block( const signed_block& b )
   {
      auto& db = _self.database();
      it = _last_blocks.find(b.block_num());
      if (it == _last_blocks.end()){
         std::vector<signed_block> blockv (1, b);
         _last_blocks.insert( std::pair<int,std::vector<signed_block>>(b.block_num(), blockv) );
         blockv.clear();
         if( _last_blocks.size() == 22)
         {
            it = _last_blocks.find(b.block_num()-21);
            blockv = it->second;
            fc::optional<signed_block> result = db.fetch_block_by_number(blockv.at(0).block_num());
            signed_block block = *result;
            if( result.valid() ){
               std::vector<signature_type> _printed_blocks;
               for(unsigned int i=0; i < blockv.size(); i++){
                  if(block.witness_signature != blockv.at(i).witness_signature){
                     if(find(_printed_blocks.begin(), _printed_blocks.end(), blockv.at(i).witness_signature) == _printed_blocks.end()){
                        idump(("Orphaned Block: ")(blockv.at(i))(blockv.at(i).block_num()));
                        _printed_blocks.push_back(blockv.at(i).witness_signature);

                     }
                  }
               }
            }
            _last_blocks.erase(it);
         }
      }
      else {
         it->second.push_back(b);
      }

   }
}

fork_info_plugin::fork_info_plugin( application* app )
   : plugin( app ), _my( new detail::fork_info_plugin_impl( *this ))
{}

fork_info_plugin::~fork_info_plugin()
{}

void fork_info_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   try
   {
      chain::database& db = database();
      db.applied_block.connect([this](const chain::signed_block& b){ _my->on_block( b ); });
   } FC_CAPTURE_AND_RETHROW()
}

void fork_info_plugin::plugin_startup()
{}
} } }
