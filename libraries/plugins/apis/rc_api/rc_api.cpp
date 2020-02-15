#include <steem/plugins/rc_api/rc_api_plugin.hpp>
#include <steem/plugins/rc_api/rc_api.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/resource_sizes.hpp>

#include <steem/chain/account_object.hpp>

#include <steem/plugins/database_api/util/iterate_results.hpp>

#include <fc/variant_object.hpp>
#include <fc/reflect/variant.hpp>

namespace steem { namespace plugins { namespace rc {

namespace detail {

using namespace database_api::util;

class rc_api_impl
{
   public:
      rc_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

      DECLARE_API_IMPL
      (
         (get_resource_params)
         (get_resource_pool)
         (find_rc_accounts)
         (list_rc_accounts)
         (find_rc_delegation_pools)
         (list_rc_delegation_pools)
         (find_rc_delegations)
         (list_rc_delegations)
      )

      chain::database& _db;
};

DEFINE_API_IMPL( rc_api_impl, get_resource_params )
{
   get_resource_params_return result;
   const rc_resource_param_object& params_obj = _db.get< rc_resource_param_object, by_id >( rc_resource_param_object::id_type() );
   fc::mutable_variant_object resource_params_mvo;

   for( size_t i=0; i<STEEM_NUM_RESOURCE_TYPES; i++ )
   {
      std::string resource_name = fc::reflector< rc_resource_types >::to_string( i );
      result.resource_names.push_back( resource_name );
      resource_params_mvo( resource_name, params_obj.resource_param_array[i] );
   }

   state_object_size_info state_size_info;
   fc::variant v_state_size_info;
   fc::to_variant( state_size_info, v_state_size_info );
   fc::mutable_variant_object state_bytes_mvo = v_state_size_info.get_object();
   state_bytes_mvo("STATE_BYTES_SCALE", STATE_BYTES_SCALE);
   // No need to expose other STATE_ constants, since they only affect state_size_info fields which are already returned

   operation_exec_info exec_info;

   fc::mutable_variant_object size_info_mvo;
   size_info_mvo
      ( "resource_state_bytes", state_bytes_mvo )
      ( "resource_execution_time", exec_info );

   result.size_info = size_info_mvo;
   result.resource_params = resource_params_mvo;

   return result;
}

DEFINE_API_IMPL( rc_api_impl, get_resource_pool )
{
   get_resource_pool_return result;
   fc::mutable_variant_object mvo;
   const rc_pool_object& pool_obj = _db.get< rc_pool_object, by_id >( rc_pool_object::id_type() );

   for( size_t i=0; i<STEEM_NUM_RESOURCE_TYPES; i++ )
   {
      resource_pool_api_object api_pool;
      api_pool.pool = pool_obj.pool_array[i];
      mvo( fc::reflector< rc_resource_types >::to_string( i ), api_pool );
   }

   result.resource_pool = mvo;
   return result;
}

DEFINE_API_IMPL( rc_api_impl, find_rc_accounts )
{
   FC_ASSERT( args.accounts.size() <= RC_API_SINGLE_QUERY_LIMIT );

   find_rc_accounts_return result;
   result.rc_accounts.reserve( args.accounts.size() );

   for( const account_name_type& a : args.accounts )
   {
      const rc_account_object* rc_account = _db.find< rc_account_object, by_name >( a );

      if( rc_account != nullptr )
      {
         result.rc_accounts.emplace_back( *rc_account, _db );
      }
   }

   return result;
}

DEFINE_API_IMPL( rc_api_impl, list_rc_accounts )
{
   FC_ASSERT( args.limit <= RC_API_SINGLE_QUERY_LIMIT );

   list_rc_accounts_return result;
   result.rc_accounts.reserve( args.limit );

   switch( args.order )
   {
      case( sort_order_type::by_name ):
      {
         iterate_results(
            _db.get_index< rc_account_index, by_name >(),
            args.start.as< account_name_type >(),
            result.rc_accounts,
            args.limit,
            [&]( const rc_account_object& rca ){ return rc_account_api_object( rca, _db ); },
            &filter_default< rc_account_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( rc_api_impl, find_rc_delegation_pools )
{
   FC_ASSERT( args.accounts.size() <= RC_API_SINGLE_QUERY_LIMIT );

   find_rc_delegation_pools_return result;
   result.rc_delegation_pools.reserve( args.accounts.size() );

   for( const auto& a : args.accounts )
   {
      const auto* pool = _db.find< rc_delegation_pool_object, by_account_symbol >( boost::make_tuple( a, VESTS_SYMBOL ) );

      if( pool != nullptr )
      {
         result.rc_delegation_pools.push_back( *pool );
      }
   }

   return result;
}

DEFINE_API_IMPL( rc_api_impl, list_rc_delegation_pools )
{
   FC_ASSERT( args.limit <= RC_API_SINGLE_QUERY_LIMIT );

   list_rc_delegation_pools_return result;
   result.rc_delegation_pools.reserve( args.limit );

   switch( args.order )
   {
      case( sort_order_type::by_name ):
      {
         iterate_results(
            _db.get_index< rc_delegation_pool_index, by_account_symbol >(),
            boost::make_tuple( args.start.as< account_name_type >(), VESTS_SYMBOL ),
            result.rc_delegation_pools,
            args.limit,
            &on_push_default< rc_delegation_pool_object >,
            &filter_default< rc_delegation_pool_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( rc_api_impl, find_rc_delegations )
{
   static_assert( STEEM_RC_MAX_INDEL <= RC_API_SINGLE_QUERY_LIMIT, "STEEM_RC_MAX_INDEL exceeds RC_API_SINGLE_QUERY_LIMIT" );

   find_rc_delegations_return result;
   result.rc_delegations.reserve( STEEM_RC_MAX_INDEL );

   const auto& del_idx = _db.get_index< rc_indel_edge_index, by_edge >();

   for( auto itr = del_idx.lower_bound( args.account ); itr != del_idx.end() && itr->from_account == args.account; ++itr )
   {
      result.rc_delegations.push_back( *itr );
   }

   return result;
}

DEFINE_API_IMPL( rc_api_impl, list_rc_delegations )
{
   FC_ASSERT( args.limit <= RC_API_SINGLE_QUERY_LIMIT );

   list_rc_delegations_return result;
   result.rc_delegations.reserve( args.limit );

   switch( args.order )
   {
      case( sort_order_type::by_edge ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 2, "by_edge start requires 2 values. (from_account, pool_name)" );

         iterate_results(
            _db.get_index< rc_indel_edge_index, by_edge >(),
            boost::make_tuple( key[0].as< account_name_type >(), VESTS_SYMBOL, key[1].as< account_name_type >() ),
            result.rc_delegations,
            args.limit,
            &on_push_default< rc_indel_edge_api_object >,
            &filter_default< rc_indel_edge_api_object > );
         break;
      }
      case( sort_order_type::by_pool ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 2, "by_edge start requires 2 values. (from_account, pool_name)" );

         iterate_results(
            _db.get_index< rc_indel_edge_index, by_pool >(),
            boost::make_tuple( key[0].as< account_name_type >(), VESTS_SYMBOL, key[1].as< account_name_type >() ),
            result.rc_delegations,
            args.limit,
            &on_push_default< rc_indel_edge_api_object >,
            &filter_default< rc_indel_edge_api_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

} // detail

rc_api::rc_api(): my( new detail::rc_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_RC_API_PLUGIN_NAME );
}

rc_api::~rc_api() {}

DEFINE_READ_APIS( rc_api,
   (get_resource_params)
   (get_resource_pool)
   (find_rc_accounts)
   (list_rc_accounts)
   (find_rc_delegation_pools)
   (list_rc_delegation_pools)
   (find_rc_delegations)
   (list_rc_delegations)
   )

} } } // steem::plugins::rc
