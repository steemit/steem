
#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/plugin/account_info/account_info_api.hpp>

namespace steemit { namespace plugin { namespace account_info {

using api = account_info_api;

namespace detail {

class account_info_api_impl
{
   public:
      account_info_api_impl( const steemit::chain::database& db );

      void get_accounts( const api::get_accounts_args& args, api::get_accounts_ret& result )const;
      void list_accounts_for_auth_members( const api::list_accounts_for_auth_members_args& args, api::list_accounts_for_auth_members_ret& result )const;
      void list_accounts( const api::list_accounts_args& args, api::list_accounts_ret& result )const;
      void get_account_count( const api::get_account_count_args& args, api::get_account_count_ret& result )const;

   private:
      const steemit::chain::database& _db;
};

account_info_api_impl::account_info_api_impl( const steemit::chain::database& db ) : _db(db) {}

void account_info_api_impl::get_accounts( const api::get_accounts_args& args, api::get_accounts_ret& result )const
{
   const auto& idx = _db.get_index_type< account_index >().indices().get< by_name >();
   result.account_objs.clear();

   for( const std::string& name : args.accounts )
   {
      auto itr = idx.find( name );
      if ( itr != idx.end() )
         result.account_objs.push_back( *itr );
   }
}

void account_info_api_impl::list_accounts_for_auth_members( const api::list_accounts_for_auth_members_args& args, api::list_accounts_for_auth_members_ret& result )const
{
   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<steemit::chain::account_member_index>();

   for( const std::string& member : args.auth_members )
   {
      set<string> s;
      std::string prefix = STEEMIT_ADDRESS_PREFIX;
      const size_t prefix_len = prefix.size();

      if( (member.length() > prefix.length())
          && (member.substr( 0, prefix_len ) == prefix)
        )
      {
         public_key_type key(member);
         auto itr = refs.account_to_key_memberships.find(key);
         if( itr != refs.account_to_key_memberships.end() )
         {
            for( auto item : itr->second )
               s.insert(item);
         }
      }
      else
      {
         /*const auto& idx = _db.get_index_type<account_index>();
         const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
         const auto& refs = aidx.get_secondary_index<steemit::chain::account_member_index>();
         auto itr = refs.account_to_account_memberships.find(account_id);
         vector<account_id_type> result;

         if( itr != refs.account_to_account_memberships.end() )
         {
            result.reserve( itr->second.size() );
            for( auto item : itr->second ) result.push_back(item);
         }
         return result;*/

         // NB this should call s.insert(name) for all account names

         FC_ASSERT( false, "Getting account references needs to be refactored for steem." );
      }

      vector< string > v;
      for( const std::string& name : s )
         v.push_back( name );
      result.accounts_by_auth_member.emplace( member, std::move( v ) );
   }
}

void account_info_api_impl::list_accounts( const api::list_accounts_args& args, api::list_accounts_ret& result )const
{
   uint32_t left = args.count;
   FC_ASSERT( left <= 1000 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();

   for( auto itr = accounts_by_name.lower_bound(args.after_account);
        left-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.names.insert(itr->name);
   }
}

void account_info_api_impl::get_account_count( const api::get_account_count_args& args, api::get_account_count_ret& result )const
{
   result.num_accounts = _db.get_index_type<account_index>().indices().size();
}


} // ns detail

account_info_api::account_info_api( const chain::database& db ) : _my( std::make_shared< detail::account_info_api_impl >( db ) ) {}
account_info_api::account_info_api( const app::application& app ) : account_info_api( *app.chain_database() ) {}

api::get_accounts_ret account_info_api::get_accounts( api::get_accounts_args args )const
{
   api::get_accounts_ret result;
   _my->get_accounts( args, result );
   return result;
}

api::list_accounts_for_auth_members_ret account_info_api::list_accounts_for_auth_members( api::list_accounts_for_auth_members_args args )const
{
   api::list_accounts_for_auth_members_ret result;
   _my->list_accounts_for_auth_members( args, result );
   return result;
}

api::list_accounts_ret account_info_api::list_accounts( api::list_accounts_args args )const
{
   api::list_accounts_ret result;
   _my->list_accounts( args, result );
   return result;
}

api::get_account_count_ret account_info_api::get_account_count( api::get_account_count_args args )const
{
   api::get_account_count_ret result;
   _my->get_account_count( args, result );
   return result;
}

} } } // ns steemit::plugin::account_info
