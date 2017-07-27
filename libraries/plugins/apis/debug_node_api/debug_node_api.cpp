#include <steemit/plugins/debug_node_api/debug_node_api_plugin.hpp>
#include <steemit/plugins/debug_node_api/debug_node_api.hpp>

#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <steemit/chain/block_log.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

namespace steemit { namespace plugins { namespace debug_node {

namespace detail {

class debug_private_key_storage : public private_key_storage
{
   public:
      debug_private_key_storage() {}
      virtual ~debug_private_key_storage() {}

      virtual void maybe_get_private_key(
         fc::optional< fc::ecc::private_key >& result,
         const steemit::chain::public_key_type& pubkey,
         const std::string& account_name
         ) override;

      std::string                dev_key_prefix;
      std::map< steemit::chain::public_key_type, fc::ecc::private_key > key_table;
};

class debug_node_api_impl
{
   public:
      debug_node_api_impl() :
         _db( appbase::app().get_plugin< chain::chain_plugin >().db() ),
         _debug_node( appbase::app().get_plugin< debug_node_plugin >() ) {}

      DECLARE_API( debug_push_blocks )
      DECLARE_API( debug_generate_blocks )
      DECLARE_API( debug_generate_blocks_until )
      DECLARE_API( debug_pop_block )
      DECLARE_API( debug_get_witness_schedule )
      DECLARE_API( debug_get_hardfork_property_object )
      DECLARE_API( debug_set_dev_key_prefix )
      void debug_get_dev_key( debug_get_dev_key_return& result, const debug_get_dev_key_args& args );
      void debug_mine( debug_mine_return& result, const debug_mine_args& args );
      DECLARE_API( debug_set_hardfork )
      DECLARE_API( debug_has_hardfork )
      void debug_get_json_schema( std::string& schema );

      chain::database& _db;
      debug_node::debug_node_plugin& _debug_node;
      debug_private_key_storage key_storage;
};

DEFINE_API( debug_node_api_impl, debug_push_blocks )
{
   auto& src_filename = args.src_filename;
   auto count = args.count;
   auto skip_validate_invariants = args.skip_validate_invariants;

   if( count == 0 )
      return { 0 };

   fc::path src_path = fc::path( src_filename );
   fc::path index_path = fc::path( src_filename + ".index" );
   if( fc::exists(src_path) && fc::exists(index_path) &&
      !fc::is_directory(src_path) && !fc::is_directory(index_path)
     )
   {
      ilog( "Loading ${n} from block_log ${fn}", ("n", count)("fn", src_filename) );
      idump( (src_filename)(count)(skip_validate_invariants) );
      steemit::chain::block_log log;
      log.open( src_path );
      uint32_t first_block = _db.head_block_num()+1;
      uint32_t skip_flags = steemit::chain::database::skip_nothing;
      if( skip_validate_invariants )
         skip_flags = skip_flags | steemit::chain::database::skip_validate_invariants;
      for( uint32_t i=0; i<count; i++ )
      {
         //fc::optional< steemit::chain::signed_block > block = log.read_block( log.get_block_pos( first_block + i ) );
         uint64_t block_pos = log.get_block_pos( first_block + i );
         if( block_pos == steemit::chain::block_log::npos )
         {
            wlog( "Block database ${fn} only contained ${i} of ${n} requested blocks", ("i", i)("n", count)("fn", src_filename) );
            return { i };
         }

         decltype( log.read_block(0) ) result;

         try
         {
            result = log.read_block( block_pos );
         }
         catch( const fc::exception& e )
         {
            elog( "Could not read block ${i} of ${n}", ("i", i)("n", count) );
            continue;
         }

         try
         {
            _db.push_block( result.first, skip_flags );
         }
         catch( const fc::exception& e )
         {
            elog( "Got exception pushing block ${bn} : ${bid} (${i} of ${n})", ("bn", result.first.block_num())("bid", result.first.id())("i", i)("n", count) );
            elog( "Exception backtrace: ${bt}", ("bt", e.to_detail_string()) );
         }
      }
      ilog( "Completed loading block_database successfully" );
      return { count };
   }
   return { 0 };
}

DEFINE_API( debug_node_api_impl, debug_generate_blocks )
{
   return { _debug_node.debug_generate_blocks( args.debug_key, args.count, chain::database::skip_nothing, 0, &key_storage ) };
}

DEFINE_API( debug_node_api_impl, debug_generate_blocks_until )
{
   return { _debug_node.debug_generate_blocks_until( args.debug_key, args.head_block_time, args.generate_sparsely, steemit::chain::database::skip_nothing, &key_storage ) };
}

DEFINE_API( debug_node_api_impl, debug_pop_block )
{
   return { _db.fetch_block_by_number( _db.head_block_num() ) };
}

DEFINE_API( debug_node_api_impl, debug_get_witness_schedule )
{
   return _db.get( steemit::chain::witness_schedule_id_type() );
}

DEFINE_API( debug_node_api_impl, debug_get_hardfork_property_object )
{
   return _db.get( steemit::chain::hardfork_property_id_type() );
}

DEFINE_API( debug_node_api_impl, debug_set_dev_key_prefix )
{
   key_storage.dev_key_prefix = args.prefix;
   return {};
}

void debug_node_api_impl::debug_get_dev_key( debug_get_dev_key_return& result, const debug_get_dev_key_args& args )
{
   fc::ecc::private_key priv = fc::ecc::private_key::regenerate( fc::sha256::hash( key_storage.dev_key_prefix + args.name ) );
   result.private_key = graphene::utilities::key_to_wif( priv );
   result.public_key = priv.get_public_key();
   return;
}

void debug_node_api_impl::debug_mine( debug_mine_return& result, const debug_mine_args& args )
{
   chain::pow2 work;
   work.input.worker_account = args.worker_account;
   work.input.prev_block = _db.head_block_id();
   _debug_node.debug_mine_work( work, _db.get_pow_summary_target() );

   chain::pow2_operation op;
   op.work = work;

   if( args.props.valid() )
      op.props = *(args.props);
   else
      op.props = _db.get_witness_schedule_object().median_props;

   const auto& acct_idx  = _db.get_index< chain::account_index >().indices().get< chain::by_name >();
   auto acct_it = acct_idx.find( args.worker_account );
   auto acct_auth = _db.find< chain::account_authority_object, chain::by_account >( args.worker_account );
   bool has_account = (acct_it != acct_idx.end());

   fc::optional< fc::ecc::private_key > priv;
   if( !has_account )
   {
      // this copies logic from get_dev_key
      priv = fc::ecc::private_key::regenerate( fc::sha256::hash( key_storage.dev_key_prefix + args.worker_account ) );
      op.new_owner_key = priv->get_public_key();
   }
   else
   {
      chain::public_key_type pubkey;
      if( acct_auth->active.key_auths.size() != 1 )
      {
         elog( "debug_mine does not understand authority for miner account ${miner}", ("miner", args.worker_account) );
      }
      FC_ASSERT( acct_auth->active.key_auths.size() == 1 );
      pubkey = acct_auth->active.key_auths.begin()->first;
      key_storage.maybe_get_private_key( priv, pubkey, args.worker_account );
   }
   FC_ASSERT( priv.valid(), "debug_node_api does not know private key for miner account ${miner}", ("miner", args.worker_account) );

   chain::signed_transaction tx;
   tx.operations.push_back(op);
   tx.ref_block_num = _db.head_block_num();
   tx.ref_block_prefix = work.input.prev_block._hash[1];
   tx.set_expiration( _db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );

   tx.sign( *priv, STEEMIT_CHAIN_ID );

   _db.push_transaction( tx );
   return;
}

DEFINE_API( debug_node_api_impl, debug_set_hardfork )
{
   if( args.hardfork_id > STEEMIT_NUM_HARDFORKS )
      return {};

   _debug_node.debug_update( [=]( chain::database& db )
   {
      db.set_hardfork( args.hardfork_id, false );
   });

   return {};
}

DEFINE_API( debug_node_api_impl, debug_has_hardfork )
{
   return { _db.get( steemit::chain::hardfork_property_id_type() ).last_hardfork >= args.hardfork_id };
}

void debug_node_api_impl::debug_get_json_schema( std::string& schema )
{
   schema = _db.get_json_schema();
}

} // detail

debug_node_api::debug_node_api()
{
   my = std::make_shared< detail::debug_node_api_impl >();

   appbase::app().get_plugin< plugins::json_rpc::json_rpc_plugin >().add_api(
      DEBUG_NODE_API_PLUGIN_NAME,
      {
         API_METHOD( debug_push_blocks ),
         API_METHOD( debug_generate_blocks ),
         API_METHOD( debug_generate_blocks_until ),
         API_METHOD( debug_pop_block ),
         API_METHOD( debug_get_witness_schedule ),
         API_METHOD( debug_get_hardfork_property_object ),
         API_METHOD( debug_set_dev_key_prefix ),
         API_METHOD( debug_get_dev_key ),
         API_METHOD( debug_mine ),
         API_METHOD( debug_set_hardfork ),
         API_METHOD( debug_has_hardfork ),
         API_METHOD( debug_get_json_schema )
      });
}

DEFINE_API( debug_node_api, debug_push_blocks )
{
   return my->debug_push_blocks( args );
}

DEFINE_API( debug_node_api, debug_generate_blocks )
{
   return my->debug_generate_blocks( args );
}

DEFINE_API( debug_node_api, debug_generate_blocks_until )
{
   return my->debug_generate_blocks_until( args );
}

DEFINE_API( debug_node_api, debug_pop_block )
{
   return my->debug_pop_block( args );
}

DEFINE_API( debug_node_api, debug_get_witness_schedule )
{
   return my->debug_get_witness_schedule( args );
}

DEFINE_API( debug_node_api, debug_get_hardfork_property_object )
{
   return my->debug_get_hardfork_property_object( args );
}

DEFINE_API( debug_node_api, debug_set_dev_key_prefix )
{
   return my->debug_set_dev_key_prefix( args );
}

DEFINE_API( debug_node_api, debug_get_dev_key )
{
   debug_get_dev_key_return result;
   my->debug_get_dev_key( result, args );
   return result;
}

DEFINE_API( debug_node_api, debug_mine )
{
   debug_mine_return result;
   my->debug_mine( result, args );
   return result;
}

DEFINE_API( debug_node_api, debug_set_hardfork )
{
   return my->debug_set_hardfork( args );
}

DEFINE_API( debug_node_api, debug_has_hardfork )
{
   return my->debug_has_hardfork( args );
}

DEFINE_API( debug_node_api, debug_get_json_schema )
{
   std::string result;
   my->debug_get_json_schema( result );
   return { result };
}

} } } // steemit::plugins::debug_node
