
#include <steem/plugins/block_data_export/block_data_export_plugin.hpp>

#include <steem/plugins/rc/rc_export_objects.hpp>
#include <steem/plugins/rc/rc_plugin.hpp>
#include <steem/plugins/rc/rc_objects.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/operation_notification.hpp>

#include <steem/jsonball/jsonball.hpp>

#define STEEM_RC_REGEN_TIME   (60*60*24*15)

namespace steem { namespace plugins { namespace rc {

using steem::plugins::block_data_export::block_data_export_plugin;

namespace detail {

using chain::plugin_exception;

class rc_plugin_impl
{
   public:
      rc_plugin_impl( rc_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void on_post_apply_transaction( const transaction_notification& note );
      void on_post_apply_block( const block_notification& note );
      void on_first_block();

      database&                     _db;
      rc_plugin&                    _self;
      boost::signals2::connection   _post_apply_block_conn;
      boost::signals2::connection   _post_apply_transaction_conn;
};

const rc_account_object& get_or_create_rc_account_object( database& db, const account_name_type& account )
{
   const rc_account_object* result = db.find< rc_account_object, by_name >( account );
   if( result != nullptr )
      return *result;

   return db.create< rc_account_object >( [&]( rc_account_object& rc_account )
   {
      rc_account.account = account;
      rc_account.rc_usage_last_update = STEEM_GENESIS_TIME;
   } );
}

struct set_creation_adjustment_visitor
{
   typedef void result_type;

   database&                                _db;
   price                                    _vesting_share_price;

   set_creation_adjustment_visitor(
      database& db,
      const price& vsp )
      : _db(db), _vesting_share_price(vsp)
   {}

   void set_creation_adjustment( const account_name_type& account, const asset& fee )const
   {
      // NB this "times" is actually division
      asset fee_vests = fee * _vesting_share_price;
      _db.modify( get_or_create_rc_account_object( _db, account ),
         [&]( rc_account_object& rc_account )
         {
            rc_account.max_rc_creation_adjustment = fee_vests;
         } );
   }

   void operator()( const account_create_operation& op )const
   {
      set_creation_adjustment( op.new_account_name, op.fee );
   }

   void operator()( const account_create_with_delegation_operation& op )const
   {
      set_creation_adjustment( op.new_account_name, op.fee );
   }

   template< typename OpType >
   void operator()( const OpType op )const {}
};

void set_creation_adjustments( database& db, const price& vsp, const signed_transaction& tx )
{
   set_creation_adjustment_visitor vtor( db, vsp );
   for( const operation& op : tx.operations )
      op.visit( vtor );
}

account_name_type get_resource_user( const signed_transaction& tx )
{
   flat_set< account_name_type > req_active;
   flat_set< account_name_type > req_owner;
   flat_set< account_name_type > req_posting;
   vector< authority > other;

   for( const operation& op : tx.operations )
   {
      operation_get_required_authorities( op, req_active, req_owner, req_posting, other );
      for( const account_name_type& account : req_active )
         return account;
      for( const account_name_type& account : req_owner )
         return account;
      for( const account_name_type& account : req_posting )
         return account;
   }
   return account_name_type();
}

void use_account_rcs(
   database& db,
   const dynamic_global_property_object& gpo,
   const account_name_type& account_name,
   int64_t rc )
{
   if( account_name == account_name_type() )
   {
      if( db.is_producing() )
      {
         STEEM_ASSERT( false, plugin_exception,
            "Tried to execute transaction with no resource user",
            );
      }
      return;
   }

   // ilog( "use_account_rcs( ${n}, ${rc} )", ("n", account_name)("rc", rc) );
   const account_object& account = db.get_account( account_name );
   const rc_account_object& rc_account = get_or_create_rc_account_object( db, account_name );

   int64_t dt = int64_t( gpo.time.sec_since_epoch() ) - int64_t( rc_account.rc_usage_last_update.sec_since_epoch() );
   FC_ASSERT( dt >= 0 );

   int64_t rc_max = account.vesting_shares.amount.value;
   rc_max += rc_account.max_rc_creation_adjustment.amount.value;

   int64_t rc_usage = rc_account.rc_usage;

   if( rc_usage > 0 )
   {
      uint128_t regen_u128 = uint64_t(dt);
      regen_u128 *= rc_max;
      regen_u128 /= STEEM_RC_REGEN_TIME;
      uint64_t regen = regen_u128.to_uint64();
      if( regen > uint64_t( rc_usage ) )
         rc_usage = 0;
      else
         rc_usage -= int64_t( regen );
   }

   int64_t rc_available = rc_max - rc_usage;

   bool has_rc = (rc <= rc_available);

   if( db.is_producing() )
   {
      STEEM_ASSERT( has_rc, plugin_exception,
         "Account: ${account} needs ${rc_needed} RC, but has ${rc_available} / ${rc_max} RC. Please wait to transact, or power up STEEM.",
         ("account", account_name)
         ("rc_needed", rc)
         ("rc_available", rc_available)
         ("rc_max", rc_max)
         );
   }

   db.modify( rc_account, [&]( rc_account_object& rca )
   {
      rca.rc_usage = rc_usage + rc;
      rca.rc_usage_last_update = gpo.time;
   } );
}

void rc_plugin_impl::on_post_apply_transaction( const transaction_notification& note )
{
   const dynamic_global_property_object& gpo = _db.get_dynamic_global_properties();
   int64_t rc_regen = gpo.total_vesting_shares.amount.value / STEEM_RC_REGEN_TIME;

   if( rc_regen < 1000 )
      return;

   rc_transaction_info tx_info;

   // How many resources does the transaction use?
   count_resources( note.transaction, tx_info.usage );

   // How many RC does this transaction cost?
   const rc_resource_param_object& params_obj = _db.get< rc_resource_param_object, by_id >( rc_resource_param_object::id_type() );
   const rc_pool_object& pool_obj = _db.get< rc_pool_object, by_id >( rc_pool_object::id_type() );

   int64_t total_cost = 0;

   for( size_t i=0; i<STEEM_NUM_RESOURCE_TYPES; i++ )
   {
      const rc_resource_params& params = params_obj.resource_param_array[i];
      int64_t pool = pool_obj.pool_array[i];

      tx_info.usage.resource_count[i] *= int64_t( params.resource_unit );
      tx_info.cost[i] = compute_rc_cost_of_resource( params.curve_params, pool, tx_info.usage.resource_count[i], rc_regen );
      total_cost += tx_info.cost[i];
   }

   // Update any accounts that were created by this transaction based on fee.
   // TODO: Add issue number to HF constant
   if( (tx_info.usage.resource_count[ resource_new_accounts ] > 0) && _db.has_hardfork( STEEM_HARDFORK_0_20 ) )
      set_creation_adjustments( _db, gpo.get_vesting_share_price(), note.transaction );

   tx_info.resource_user = get_resource_user( note.transaction );
   use_account_rcs( _db, gpo, tx_info.resource_user, total_cost );

   std::shared_ptr< exp_rc_data > export_data =
      steem::plugins::block_data_export::find_export_data< exp_rc_data >( STEEM_RC_PLUGIN_NAME );
   if( export_data )
      export_data->tx_info.push_back( tx_info );
}

void rc_plugin_impl::on_post_apply_block( const block_notification& note )
{
   const dynamic_global_property_object& gpo = _db.get_dynamic_global_properties();

   if( gpo.head_block_number == 1 )
   {
      on_first_block();
   }

   if( gpo.total_vesting_shares.amount <= 0 )
   {
      return;
   }

   // How many resources did transactions use?
   count_resources_result count;
   for( const signed_transaction& tx : note.block.transactions )
   {
      count_resources( tx, count );
   }

   const rc_resource_param_object& params_obj = _db.get< rc_resource_param_object, by_id >( rc_resource_param_object::id_type() );

   rc_block_info block_info;

   _db.modify( _db.get< rc_pool_object, by_id >( rc_pool_object::id_type() ),
      [&]( rc_pool_object& pool_obj )
      {
         for( size_t i=0; i<STEEM_NUM_RESOURCE_TYPES; i++ )
         {
            const rc_resource_params& params = params_obj.resource_param_array[i];
            int64_t& pool = pool_obj.pool_array[i];
            uint32_t dt = 0;

            block_info.pool[i] = pool;
            switch( params.time_unit )
            {
               case rc_time_unit_blocks:
                  dt = 1;
                  break;
               case rc_time_unit_seconds:
                  dt = gpo.time.sec_since_epoch() - pool_obj.last_update.sec_since_epoch();
                  break;
               default:
                  FC_ASSERT( false, "unknown time unit in RC parameter object" );
            }
            block_info.dt[i] = dt;

            block_info.decay[i] = compute_pool_decay( params.decay_params, pool, dt );
            block_info.budget[i] = int64_t( params.budget_per_time_unit ) * int64_t( dt );
            block_info.usage[i] = count.resource_count[i]*int64_t( params.resource_unit );

            pool = pool - block_info.decay[i] + block_info.budget[i] - block_info.usage[i];
         }
         pool_obj.last_update = gpo.time;
      } );

   std::shared_ptr< exp_rc_data > export_data =
      steem::plugins::block_data_export::find_export_data< exp_rc_data >( STEEM_RC_PLUGIN_NAME );
   if( export_data )
      export_data->block_info = block_info;
}

void rc_plugin_impl::on_first_block()
{
   std::string resource_params_json = steem::jsonball::get_resource_parameters();
   fc::variant resource_params_var = fc::json::from_string( resource_params_json, fc::json::strict_parser );
   std::vector< std::pair< fc::variant, std::pair< fc::variant_object, fc::variant_object > > > resource_params_pairs;
   fc::from_variant( resource_params_var, resource_params_pairs );

   _db.create< rc_resource_param_object >(
      [&]( rc_resource_param_object& params_obj )
      {
         for( auto& kv : resource_params_pairs )
         {
            auto k = kv.first.as< rc_resource_types >();
            fc::variant_object& vo = kv.second.first;
            fc::mutable_variant_object mvo(vo);
            mvo["time_unit"] = int8_t( vo["time_unit"].as< rc_time_unit_type >() );
            fc::from_variant( fc::variant( mvo ), params_obj.resource_param_array[ k ] );
         }

         ilog( "Genesis params_obj is ${o}", ("o", params_obj) );
      } );

   const rc_resource_param_object& params_obj = _db.get< rc_resource_param_object, by_id >( rc_resource_param_object::id_type() );

   _db.create< rc_pool_object >(
      [&]( rc_pool_object& pool_obj )
      {
         for( size_t i=0; i<STEEM_NUM_RESOURCE_TYPES; i++ )
         {
            const rc_resource_params& params = params_obj.resource_param_array[i];
            pool_obj.pool_array[i] = params.pool_eq;
         }
         pool_obj.last_update = _db.get_dynamic_global_properties().time;

         ilog( "Genesis pool_obj is ${o}", ("o", pool_obj) );
      } );
   return;
}

} // detail

rc_plugin::rc_plugin() {}
rc_plugin::~rc_plugin() {}

void rc_plugin::set_program_options( options_description& cli, options_description& cfg ){}

void rc_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   ilog( "Initializing resource credit plugin" );

   my = std::make_unique< detail::rc_plugin_impl >( *this );

   try
   {
      block_data_export_plugin* export_plugin = appbase::app().find_plugin< block_data_export_plugin >();
      if( export_plugin != nullptr )
      {
         ilog( "Registering RC export data factory" );
         export_plugin->register_export_data_factory( STEEM_RC_PLUGIN_NAME,
            []() -> std::shared_ptr< exportable_block_data > { return std::make_shared< exp_rc_data >(); } );
      }

      chain::database& db = appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();

      my->_post_apply_block_conn = db.add_post_apply_block_handler( [&]( const block_notification& note ){ my->on_post_apply_block( note ); }, *this, 0 );
      my->_post_apply_transaction_conn = db.add_post_apply_transaction_handler( [&]( const transaction_notification& note ){ my->on_post_apply_transaction( note ); }, *this, 0 );

      add_plugin_index< rc_resource_param_index >(db);
      add_plugin_index< rc_pool_index >(db);
      add_plugin_index< rc_account_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void rc_plugin::plugin_startup() {}

void rc_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_post_apply_block_conn );
   chain::util::disconnect_signal( my->_post_apply_transaction_conn );
}

exp_rc_data::exp_rc_data() {}
exp_rc_data::~exp_rc_data() {}

void exp_rc_data::to_variant( fc::variant& v )const
{
   fc::to_variant( *this, v );
}

} } } // steem::plugins::rc
