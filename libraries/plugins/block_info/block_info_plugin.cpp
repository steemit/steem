
#include <steemit/chain/database.hpp>
#include <steemit/chain/global_property_object.hpp>

#include <steemit/plugins/block_info/block_info.hpp>
#include <steemit/plugins/block_info/block_info_api.hpp>
#include <steemit/plugins/block_info/block_info_plugin.hpp>

#include <string>

namespace steemit { namespace plugin { namespace block_info {

block_info_plugin::block_info_plugin( application* app ) : plugin( app ) {}
block_info_plugin::~block_info_plugin() {}

std::string block_info_plugin::plugin_name()const
{
   return "block_info";
}

void block_info_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   chain::database& db = database();

   _applied_block_conn  = db.applied_block.connect([this](const chain::signed_block& b){ on_applied_block(b); });
}

void block_info_plugin::plugin_startup()
{
   app().register_api_factory< block_info_api >( "block_info_api" );
}

void block_info_plugin::plugin_shutdown()
{
}

void block_info_plugin::on_applied_block( const chain::signed_block& b )
{
   uint32_t block_num = b.block_num();
   const chain::database& db = database();

   while( block_num >= _block_info.size() )
      _block_info.emplace_back();

   block_info& info = _block_info[block_num];
   const chain::dynamic_global_property_object& dgpo = db.get_dynamic_global_properties();

   info.block_id                    = b.id();
   info.block_size                  = fc::raw::pack_size( b );
   info.aslot                       = dgpo.current_aslot;
   info.last_irreversible_block_num = dgpo.last_irreversible_block_num;
   info.num_pow_witnesses           = dgpo.num_pow_witnesses;
   return;
}

} } } // steemit::plugin::block_info

STEEMIT_DEFINE_PLUGIN( block_info, steemit::plugin::block_info::block_info_plugin )
