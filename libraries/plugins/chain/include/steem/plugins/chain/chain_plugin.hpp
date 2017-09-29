#pragma once
#include <appbase/application.hpp>
#include <steem/chain/database.hpp>

#include <boost/signals2.hpp>

#define STEEM_CHAIN_PLUGIN_NAME "chain"

namespace steem { namespace plugins { namespace chain {

namespace detail { class chain_plugin_impl; }

using std::unique_ptr;
using namespace appbase;
using namespace steem::chain;

class chain_plugin : public plugin< chain_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES()

   chain_plugin();
   virtual ~chain_plugin();

   static const std::string& name() { static std::string name = STEEM_CHAIN_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;
   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   bool accept_block( const steem::chain::signed_block& block, bool currently_syncing, uint32_t skip );
   void accept_transaction( const steem::chain::signed_transaction& trx );

   bool block_is_on_preferred_chain( const steem::chain::block_id_type& block_id );

   void check_time_in_block( const steem::chain::signed_block& block );

   template< typename MultiIndexType >
   bool has_index() const
   {
      return db().has_index< MultiIndexType >();
   }

   template< typename MultiIndexType >
   const chainbase::generic_index< MultiIndexType >& get_index() const
   {
      return db().get_index< MultiIndexType >();
   }

   template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
   const ObjectType* find( CompatibleKey&& key ) const
   {
      return db().find< ObjectType, IndexedByType, CompatibleKey >( key );
   }

   template< typename ObjectType >
   const ObjectType* find( chainbase::oid< ObjectType > key = chainbase::oid< ObjectType >() )
   {
      return db().find< ObjectType >( key );
   }

   template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
   const ObjectType& get( CompatibleKey&& key ) const
   {
      return db().get< ObjectType, IndexedByType, CompatibleKey >( key );
   }

   template< typename ObjectType >
   const ObjectType& get( const chainbase::oid< ObjectType >& key = chainbase::oid< ObjectType >() )
   {
      return db().get< ObjectType >( key );
   }

   // Exposed for backwards compatibility. In the future, plugins should manage their own internal database
   database& db();
   const database& db() const;

   // Emitted when the blockchain is syncing/live.
   // This is to synchronize plugins that have the chain plugin as an optional dependency.
   boost::signals2::signal<void()> on_sync;

private:
   std::unique_ptr< detail::chain_plugin_impl > my;
};

} } } // steem::plugins::chain
