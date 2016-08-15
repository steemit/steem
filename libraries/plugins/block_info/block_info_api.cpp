
#include <steemit/app/api_context.hpp>
#include <steemit/app/application.hpp>

#include <steemit/plugins/block_info/block_info_api.hpp>
#include <steemit/plugins/block_info/block_info_plugin.hpp>

namespace steemit { namespace plugin { namespace block_info {

namespace detail {

class block_info_api_impl
{
   public:
      block_info_api_impl( steemit::app::application& _app );

      std::shared_ptr< steemit::plugin::block_info::block_info_plugin > get_plugin();

      void get_block_info( const get_block_info_args& args, std::vector< block_info >& result );
      void get_blocks_with_info( const get_block_info_args& args, std::vector< block_with_info >& result );

      steemit::app::application& app;
};

block_info_api_impl::block_info_api_impl( steemit::app::application& _app ) : app( _app )
{}

std::shared_ptr< steemit::plugin::block_info::block_info_plugin > block_info_api_impl::get_plugin()
{
   return app.get_plugin< block_info_plugin >( "block_info" );
}

void block_info_api_impl::get_block_info( const get_block_info_args& args, std::vector< block_info >& result )
{
   const std::vector< block_info >& _block_info = get_plugin()->_block_info;

   FC_ASSERT( args.start_block_num > 0 );
   FC_ASSERT( args.count <= 10000 );
   uint32_t n = std::min( uint32_t( _block_info.size() ), args.start_block_num + args.count );
   for( uint32_t block_num=args.start_block_num; block_num<n; block_num++ )
      result.emplace_back( _block_info[block_num] );
   return;
}

void block_info_api_impl::get_blocks_with_info( const get_block_info_args& args, std::vector< block_with_info >& result )
{
   const std::vector< block_info >& _block_info = get_plugin()->_block_info;
   const chain::database& db = get_plugin()->database();

   FC_ASSERT( args.start_block_num > 0 );
   FC_ASSERT( args.count <= 10000 );
   uint32_t n = std::min( uint32_t( _block_info.size() ), args.start_block_num + args.count );
   uint64_t total_size = 0;
   for( uint32_t block_num=args.start_block_num; block_num<n; block_num++ )
   {
      uint64_t new_size = total_size + _block_info[block_num].block_size;
      if( (new_size > 8*1024*1024) && (block_num != args.start_block_num) )
         break;
      total_size = new_size;
      result.emplace_back();
      result.back().block = *db.fetch_block_by_number(block_num);
      result.back().info = _block_info[block_num];
   }
   return;
}

} // detail

block_info_api::block_info_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::block_info_api_impl >(ctx.app);
}

std::vector< block_info > block_info_api::get_block_info( get_block_info_args args )
{
   std::vector< block_info > result;
   my->get_block_info( args, result );
   return result;
}

std::vector< block_with_info > block_info_api::get_blocks_with_info( get_block_info_args args )
{
   std::vector< block_with_info > result;
   my->get_blocks_with_info( args, result );
   return result;
}

void block_info_api::on_api_startup() { }

} } } // steemit::plugin::block_info
