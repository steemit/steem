#pragma once
#include <steem/chain/steem_fwd.hpp>
#include <appbase/application.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace block_data_export {

namespace detail { class block_data_export_plugin_impl; }

using namespace appbase;

#define STEEM_BLOCK_DATA_EXPORT_PLUGIN_NAME "block_data_export"

class exportable_block_data;

class block_data_export_plugin : public appbase::plugin< block_data_export_plugin >
{
   public:
      block_data_export_plugin();
      virtual ~block_data_export_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_BLOCK_DATA_EXPORT_PLUGIN_NAME; return name; }

      virtual void set_program_options( options_description& cli, options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      void register_export_data_factory( const std::string& name, std::function< std::shared_ptr< exportable_block_data >() >& factory );

      template< typename Callable >
      void register_export_data_factory( const std::string& name, Callable lamb )
      {
         std::function< std::shared_ptr< exportable_block_data >() > func( lamb );
         register_export_data_factory( name, func );
      }

      void add_abstract_export_data( const std::string& name, std::shared_ptr< exportable_block_data > data );
      std::shared_ptr< exportable_block_data > find_abstract_export_data( const std::string& name );

      template< typename T >
      std::shared_ptr< T > find_export_data( const std::string& name )
      {
         std::shared_ptr< exportable_block_data > adata = find_abstract_export_data( name );
         if( !adata )
            return std::shared_ptr<T>();
         std::shared_ptr< T > result = std::dynamic_pointer_cast< T >( adata );
         FC_ASSERT( result, "Could not dynamically cast export data" );
         return result;
      }

      template< typename T, typename CtorArg >
      std::shared_ptr< T > get_or_create_export_data( const std::string& name, const CtorArg& arg )
      {
         std::shared_ptr< T > result = find_export_data<T>( name );
         if( !result )
         {
            result = std::make_shared< T >( arg );
            add_abstract_export_data( name, result );
         }
         return result;
      }

      template< typename T >
      void register_export_data_type( const std::string& name )
      {
         register_export_data_factory( name, []() -> std::shared_ptr< exportable_block_data > { return std::make_shared<T>(); } );
      }

   private:
      std::unique_ptr< detail::block_data_export_plugin_impl > my;
};

template< typename T >
std::shared_ptr< T > find_export_data( const std::string& name )
{
   block_data_export_plugin* export_plugin = appbase::app().find_plugin< block_data_export_plugin >();
   if( export_plugin == nullptr )
      return std::shared_ptr< T >();
   return export_plugin->find_export_data< T >( name );
}

} } } // steem::plugins::block_data_export
