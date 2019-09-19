#include <steem/chain/statefile/statefile.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>

namespace steem { namespace chain { namespace statefile {

// db_format_version : Must match STEEM_DB_FORMAT_VERSION
// network_type      : Must match STEEM_NETWORK_TYPE
// chain_id          : Must match requested chain ID and value of embedded GPO

steem_version_info::steem_version_info( const database& db )
{
   db_format_version = STEEM_DB_FORMAT_VERSION;
   network_type = STEEM_NETWORK_TYPE;
   db.for_each_index_extension< index_info >( [&]( std::shared_ptr< index_info > info )
   {
      std::shared_ptr< abstract_schema > sch = info->get_schema();
      std::string schema_name, str_schema;
      sch->get_name( schema_name );
      sch->get_str_schema( str_schema );
      object_schemas.emplace( schema_name, str_schema );
   } );

   chain_id = db.get_chain_id().str();
   head_block_num = db.revision();
}

} } } // steem::chain::statefile
