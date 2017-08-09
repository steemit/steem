#pragma once
#include <appbase/application.hpp>
#include <steemit/chain/database.hpp>

#define STEEM_CHAIN_PLUGIN_NAME "chain"

namespace steemit { namespace plugins { namespace chain {

using std::unique_ptr;
using namespace appbase;
using namespace steemit::chain;

class chain_plugin : public plugin< chain_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES()

   chain_plugin();
   virtual ~chain_plugin();

   static std::string& name() { static std::string name = STEEM_CHAIN_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;
   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

   bool accept_block( const steemit::chain::signed_block& block, bool currently_syncing );
   void accept_transaction( const steemit::chain::signed_transaction& trx );

   bool block_is_on_preferred_chain( const steemit::chain::block_id_type& block_id );

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

private:
   unique_ptr<class chain_plugin_impl> my;
};

} } } // steemit::plugins::chain
