#include <steemit/plugins/account_by_key/account_by_key_plugin.hpp>
#include <steemit/plugins/account_by_key/account_by_key_objects.hpp>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/operation_notification.hpp>

#include <graphene/schema/schema.hpp>
#include <graphene/schema/schema_impl.hpp>

namespace steem { namespace plugins { namespace account_by_key {

namespace detail {

class account_by_key_plugin_impl
{
   public:
      account_by_key_plugin_impl( account_by_key_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );
      void clear_cache();
      void cache_auths( const account_authority_object& a );
      void update_key_lookup( const account_authority_object& a );

      flat_set< public_key_type >   cached_keys;
      database&                     _db;
      account_by_key_plugin&        _self;
      boost::signals2::connection   pre_apply_connection;
      boost::signals2::connection   post_apply_connection;
};

struct pre_operation_visitor
{
   account_by_key_plugin_impl& _plugin;

   pre_operation_visitor( account_by_key_plugin_impl& plugin ) : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const account_create_operation& op )const
   {
      _plugin.clear_cache();
   }

   void operator()( const account_create_with_delegation_operation& op )const
   {
      _plugin.clear_cache();
   }

   void operator()( const account_update_operation& op )const
   {
      _plugin.clear_cache();
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.account );
      if( acct_itr ) _plugin.cache_auths( *acct_itr );
   }

   void operator()( const recover_account_operation& op )const
   {
      _plugin.clear_cache();
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.account_to_recover );
      if( acct_itr ) _plugin.cache_auths( *acct_itr );
   }

   void operator()( const pow_operation& op )const
   {
      _plugin.clear_cache();
   }

   void operator()( const pow2_operation& op )const
   {
      _plugin.clear_cache();
   }
};

struct pow2_work_get_account_visitor
{
   typedef const account_name_type* result_type;

   template< typename WorkType >
   result_type operator()( const WorkType& work )const
   {
      return &work.input.worker_account;
   }
};

struct post_operation_visitor
{
   account_by_key_plugin_impl& _plugin;

   post_operation_visitor( account_by_key_plugin_impl& plugin ) : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const account_create_operation& op )const
   {
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.new_account_name );
      if( acct_itr ) _plugin.update_key_lookup( *acct_itr );
   }

   void operator()( const account_create_with_delegation_operation& op )const
   {
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.new_account_name );
      if( acct_itr ) _plugin.update_key_lookup( *acct_itr );
   }

   void operator()( const account_update_operation& op )const
   {
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.account );
      if( acct_itr ) _plugin.update_key_lookup( *acct_itr );
   }

   void operator()( const recover_account_operation& op )const
   {
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.account_to_recover );
      if( acct_itr ) _plugin.update_key_lookup( *acct_itr );
   }

   void operator()( const pow_operation& op )const
   {
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( op.worker_account );
      if( acct_itr ) _plugin.update_key_lookup( *acct_itr );
   }

   void operator()( const pow2_operation& op )const
   {
      const account_name_type* worker_account = op.work.visit( pow2_work_get_account_visitor() );
      if( worker_account == nullptr )
         return;
      auto acct_itr = _plugin._db.find< account_authority_object, by_account >( *worker_account );
      if( acct_itr ) _plugin.update_key_lookup( *acct_itr );
   }

   void operator()( const hardfork_operation& op )const
   {
      if( op.hardfork_id == STEEM_HARDFORK_0_9 )
      {
         auto& db = _plugin._db;

         for( const std::string& acc : hardfork9::get_compromised_accounts() )
         {
            const account_object* account = db.find_account( acc );
            if( account == nullptr )
               continue;

            db.create< key_lookup_object >( [&]( key_lookup_object& o )
            {
               o.key = public_key_type( "STM7sw22HqsXbz7D2CmJfmMwt9rimtk518dRzsR1f8Cgw52dQR1pR" );
               o.account = account->name;
            });
         }
      }
   }
};

void account_by_key_plugin_impl::clear_cache()
{
   cached_keys.clear();
}

void account_by_key_plugin_impl::cache_auths( const account_authority_object& a )
{
   for( const auto& item : a.owner.key_auths )
      cached_keys.insert( item.first );
   for( const auto& item : a.active.key_auths )
      cached_keys.insert( item.first );
   for( const auto& item : a.posting.key_auths )
      cached_keys.insert( item.first );
}

void account_by_key_plugin_impl::update_key_lookup( const account_authority_object& a )
{
   flat_set< public_key_type > new_keys;

   // Construct the set of keys in the account's authority
   for( const auto& item : a.owner.key_auths )
      new_keys.insert( item.first );
   for( const auto& item : a.active.key_auths )
      new_keys.insert( item.first );
   for( const auto& item : a.posting.key_auths )
      new_keys.insert( item.first );

   // For each key that needs a lookup
   for( const auto& key : new_keys )
   {
      // If the key was not in the authority, add it to the lookup
      if( cached_keys.find( key ) == cached_keys.end() )
      {
         auto lookup_itr = _db.find< key_lookup_object, by_key >( std::make_tuple( key, a.account ) );

         if( lookup_itr == nullptr )
         {
            _db.create< key_lookup_object >( [&]( key_lookup_object& o )
            {
               o.key = key;
               o.account = a.account;
            });
         }
      }
      else
      {
         // If the key was already in the auths, remove it from the set so we don't delete it
         cached_keys.erase( key );
      }
   }

   // Loop over the keys that were in authority but are no longer and remove them from the lookup
   for( const auto& key : cached_keys )
   {
      auto lookup_itr = _db.find< key_lookup_object, by_key >( std::make_tuple( key, a.account ) );

      if( lookup_itr != nullptr )
      {
         _db.remove( *lookup_itr );
      }
   }

   cached_keys.clear();
}

void account_by_key_plugin_impl::pre_operation( const operation_notification& note )
{
   note.op.visit( pre_operation_visitor( *this ) );
}

void account_by_key_plugin_impl::post_operation( const operation_notification& note )
{
   note.op.visit( post_operation_visitor( *this ) );
}

} // detail

account_by_key_plugin::account_by_key_plugin() {}
account_by_key_plugin::~account_by_key_plugin() {}

void account_by_key_plugin::set_program_options( options_description& cli, options_description& cfg ){}

void account_by_key_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::account_by_key_plugin_impl >( *this );
   try
   {
      ilog( "Initializing account_by_key plugin" );
      chain::database& db = appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();

      my->pre_apply_connection = db.pre_apply_operation.connect( [&]( const operation_notification& o ){ my->pre_operation( o ); } );
      my->post_apply_connection = db.post_apply_operation.connect( [&]( const operation_notification& o ){ my->post_operation( o ); } );

      add_plugin_index< key_lookup_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void account_by_key_plugin::plugin_startup() {}

void account_by_key_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->pre_apply_connection );
   chain::util::disconnect_signal( my->post_apply_connection );
}

} } } // steem::plugins::account_by_key
