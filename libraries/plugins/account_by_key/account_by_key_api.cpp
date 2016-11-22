#include <steemit/account_by_key/account_by_key_api.hpp>
#include <steemit/account_by_key/account_by_key_objects.hpp>

namespace steemit { namespace account_by_key {

namespace detail
{

class account_by_key_api_impl
{
   public:
      account_by_key_api_impl( steemit::app::application& app )
         :_app( app ) {}

      vector< vector< account_name_type > > get_key_references( vector< public_key_type >& keys )const;

      steemit::app::application& _app;
};

vector< vector< account_name_type > > account_by_key_api_impl::get_key_references( vector< public_key_type >& keys )const
{
   vector< vector< account_name_type > > final_result;
   final_result.reserve( keys.size() );

   const auto& key_idx = _app.chain_database()->get_index< key_lookup_index >().indices().get< by_key >();

   for( auto& key : keys )
   {
      vector< account_name_type > result;
      auto lookup_itr = key_idx.lower_bound( key );

      while( lookup_itr != key_idx.end() && lookup_itr->key == key )
      {
         result.push_back( lookup_itr->account );
         ++lookup_itr;
      }

      final_result.emplace_back( std::move( result ) );
   }

   return final_result;
}

} // detail

account_by_key_api::account_by_key_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::account_by_key_api_impl >( ctx.app );
}

void account_by_key_api::on_api_startup() {}

vector< vector< account_name_type > > account_by_key_api::get_key_references( vector< public_key_type > keys )const
{
   return my->_app.chain_database()->with_read_lock( [&]()
   {
      return my->get_key_references( keys );
   });
}

} } // steemit::account_by_key
