#include <steem/chain/steem_fwd.hpp>

#include <appbase/application.hpp>

#include <steem/plugins/database_api/database_api.hpp>
#include <steem/plugins/database_api/database_api_plugin.hpp>

#include <steem/protocol/get_config.hpp>
#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/transaction_util.hpp>

#include <steem/chain/util/smt_token.hpp>

#include <steem/utilities/git_revision.hpp>

#include <fc/git_revision.hpp>

namespace steem { namespace plugins { namespace database_api {

class database_api_impl
{
   public:
      database_api_impl();
      ~database_api_impl();

      DECLARE_API_IMPL
      (
         (get_config)
         (get_version)
         (get_dynamic_global_properties)
         (get_witness_schedule)
         (get_hardfork_properties)
         (get_reward_funds)
         (get_current_price_feed)
         (get_feed_history)
         (list_witnesses)
         (find_witnesses)
         (list_witness_votes)
         (get_active_witnesses)
         (list_accounts)
         (find_accounts)
         (list_owner_histories)
         (find_owner_histories)
         (list_account_recovery_requests)
         (find_account_recovery_requests)
         (list_change_recovery_account_requests)
         (find_change_recovery_account_requests)
         (list_escrows)
         (find_escrows)
         (list_withdraw_vesting_routes)
         (find_withdraw_vesting_routes)
         (list_savings_withdrawals)
         (find_savings_withdrawals)
         (list_vesting_delegations)
         (find_vesting_delegations)
         (list_vesting_delegation_expirations)
         (find_vesting_delegation_expirations)
         (list_sbd_conversion_requests)
         (find_sbd_conversion_requests)
         (list_decline_voting_rights_requests)
         (find_decline_voting_rights_requests)
         (list_comments)
         (find_comments)
         (list_votes)
         (find_votes)
         (list_limit_orders)
         (find_limit_orders)
         (get_order_book)
         (get_transaction_hex)
         (get_required_signatures)
         (get_potential_signatures)
         (verify_authority)
         (verify_account_authority)
         (verify_signatures)
#ifdef STEEM_ENABLE_SMT
         (get_nai_pool)
         (list_smt_tokens)
         (find_smt_tokens)
         (list_smt_token_emissions)
         (find_smt_token_emissions)
#endif
      )

      template< typename ResultType >
      static ResultType on_push_default( const ResultType& r ) { return r; }

      template< typename IndexType, typename OrderType, typename ValueType, typename ResultType, typename OnPush >
      void iterate_results( ValueType start, vector< ResultType >& result, uint32_t limit, OnPush&& on_push = &database_api_impl::on_push_default< ResultType > )
      {
         const auto& idx = _db.get_index< IndexType, OrderType >();
         auto itr = idx.lower_bound( start );
         auto end = idx.end();

         while( result.size() < limit && itr != end )
         {
            result.push_back( on_push( *itr ) );
            ++itr;
         }
      }

      chain::database& _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api()
   : my( new database_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_DATABASE_API_PLUGIN_NAME );
}

database_api::~database_api() {}

database_api_impl::database_api_impl()
   : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

database_api_impl::~database_api_impl() {}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////


DEFINE_API_IMPL( database_api_impl, get_config )
{
   return steem::protocol::get_config();
}

DEFINE_API_IMPL( database_api_impl, get_version )
{
   return get_version_return
   (
      fc::string( STEEM_BLOCKCHAIN_VERSION ),
      fc::string( steem::utilities::git_revision_sha ),
      fc::string( fc::git_revision_sha ),
      _db.get_chain_id()
   );
}

DEFINE_API_IMPL( database_api_impl, get_dynamic_global_properties )
{
   return _db.get_dynamic_global_properties();
}

DEFINE_API_IMPL( database_api_impl, get_witness_schedule )
{
   return api_witness_schedule_object( _db.get_witness_schedule_object() );
}

DEFINE_API_IMPL( database_api_impl, get_hardfork_properties )
{
   return _db.get_hardfork_property_object();
}

DEFINE_API_IMPL( database_api_impl, get_reward_funds )
{
   get_reward_funds_return result;

   const auto& rf_idx = _db.get_index< reward_fund_index, by_id >();
   auto itr = rf_idx.begin();

   while( itr != rf_idx.end() )
   {
      result.funds.push_back( *itr );
      ++itr;
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_current_price_feed )
{
   return _db.get_feed_history().current_median_history;;
}

DEFINE_API_IMPL( database_api_impl, get_feed_history )
{
   return _db.get_feed_history();
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, list_witnesses )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_witnesses_return result;
   result.witnesses.reserve( args.limit );

   switch( args.order )
   {
      case( by_name ):
      {
         iterate_results< chain::witness_index, chain::by_name >(
            args.start.as< protocol::account_name_type >(),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      case( by_vote_name ):
      {
         auto key = args.start.as< std::pair< share_type, account_name_type > >();
         iterate_results< chain::witness_index, chain::by_vote_name >(
            boost::make_tuple( key.first, key.second ),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      case( by_schedule_time ):
      {
         auto key = args.start.as< std::pair< fc::uint128, account_name_type > >();
         auto wit_id = _db.get< chain::witness_object, chain::by_name >( key.second ).id;
         iterate_results< chain::witness_index, chain::by_schedule_time >(
            boost::make_tuple( key.first, wit_id ),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_witnesses )
{
   FC_ASSERT( args.owners.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   find_witnesses_return result;

   for( auto& o : args.owners )
   {
      auto witness = _db.find< chain::witness_object, chain::by_name >( o );

      if( witness != nullptr )
         result.witnesses.push_back( api_witness_object( *witness ) );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, list_witness_votes )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_witness_votes_return result;
   result.votes.reserve( args.limit );

   switch( args.order )
   {
      case( by_account_witness ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::witness_vote_index, chain::by_account_witness >(
            boost::make_tuple( key.first, key.second ),
            result.votes,
            args.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); } );
         break;
      }
      case( by_witness_account ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::witness_vote_index, chain::by_witness_account >(
            boost::make_tuple( key.first, key.second ),
            result.votes,
            args.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_active_witnesses )
{
   const auto& wso = _db.get_witness_schedule_object();
   size_t n = wso.current_shuffled_witnesses.size();
   get_active_witnesses_return result;
   result.witnesses.reserve( n );
   for( size_t i=0; i<n; i++ )
      result.witnesses.push_back( wso.current_shuffled_witnesses[i] );
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

/* Accounts */

DEFINE_API_IMPL( database_api_impl, list_accounts )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_accounts_return result;
   result.accounts.reserve( args.limit );

   switch( args.order )
   {
      case( by_name ):
      {
         iterate_results< chain::account_index, chain::by_name >(
            args.start.as< protocol::account_name_type >(),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_proxy ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::account_index, chain::by_proxy >(
            boost::make_tuple( key.first, key.second ),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_next_vesting_withdrawal ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::account_index, chain::by_next_vesting_withdrawal >(
            boost::make_tuple( key.first, key.second ),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_accounts )
{
   find_accounts_return result;
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto acct = _db.find< chain::account_object, chain::by_name >( a );
      if( acct != nullptr )
         result.accounts.push_back( api_account_object( *acct, _db ) );
   }

   return result;
}


/* Owner Auth Histories */

DEFINE_API_IMPL( database_api_impl, list_owner_histories )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_owner_histories_return result;
   result.owner_auths.reserve( args.limit );

   auto key = args.start.as< std::pair< account_name_type, fc::time_point_sec > >();
   iterate_results< chain::owner_authority_history_index, chain::by_account >(
      boost::make_tuple( key.first, key.second ),
      result.owner_auths,
      args.limit,
      [&]( const owner_authority_history_object& o ){ return api_owner_authority_history_object( o ); } );

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_owner_histories )
{
   find_owner_histories_return result;

   const auto& hist_idx = _db.get_index< chain::owner_authority_history_index, chain::by_account >();
   auto itr = hist_idx.lower_bound( args.owner );

   while( itr != hist_idx.end() && itr->account == args.owner && result.owner_auths.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.owner_auths.push_back( api_owner_authority_history_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Account Recovery Requests */

DEFINE_API_IMPL( database_api_impl, list_account_recovery_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_account_recovery_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_account ):
      {
         iterate_results< chain::account_recovery_request_index, chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); } );
         break;
      }
      case( by_expiration ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::account_recovery_request_index, chain::by_expiration >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_account_recovery_requests )
{
   find_account_recovery_requests_return result;
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto request = _db.find< chain::account_recovery_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( api_account_recovery_request_object( *request ) );
   }

   return result;
}


/* Change Recovery Account Requests */

DEFINE_API_IMPL( database_api_impl, list_change_recovery_account_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_change_recovery_account_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_account ):
      {
         iterate_results< chain::change_recovery_account_request_index, chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< change_recovery_account_request_object > );
         break;
      }
      case( by_effective_date ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::change_recovery_account_request_index, chain::by_effective_date >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< change_recovery_account_request_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_change_recovery_account_requests )
{
   find_change_recovery_account_requests_return result;
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto request = _db.find< chain::change_recovery_account_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( *request );
   }

   return result;
}


/* Escrows */

DEFINE_API_IMPL( database_api_impl, list_escrows )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_escrows_return result;
   result.escrows.reserve( args.limit );

   switch( args.order )
   {
      case( by_from_id ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::escrow_index, chain::by_from_id >(
            boost::make_tuple( key.first, key.second ),
            result.escrows,
            args.limit,
            &database_api_impl::on_push_default< escrow_object > );
         break;
      }
      case( by_ratification_deadline ):
      {
         auto key = args.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_ratification_deadline start requires 3 values. (bool, time_point_sec, escrow_id_type)" );
         iterate_results< chain::escrow_index, chain::by_ratification_deadline >(
            boost::make_tuple( key[0].as< bool >(), key[1].as< fc::time_point_sec >(), key[2].as< escrow_id_type >() ),
            result.escrows,
            args.limit,
            &database_api_impl::on_push_default< escrow_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_escrows )
{
   find_escrows_return result;

   const auto& escrow_idx = _db.get_index< chain::escrow_index, chain::by_from_id >();
   auto itr = escrow_idx.lower_bound( args.from );

   while( itr != escrow_idx.end() && itr->from == args.from && result.escrows.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.escrows.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Withdraw Vesting Routes */

DEFINE_API_IMPL( database_api_impl, list_withdraw_vesting_routes )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_withdraw_vesting_routes_return result;
   result.routes.reserve( args.limit );

   switch( args.order )
   {
      case( by_withdraw_route ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::withdraw_vesting_route_index, chain::by_withdraw_route >(
            boost::make_tuple( key.first, key.second ),
            result.routes,
            args.limit,
            &database_api_impl::on_push_default< withdraw_vesting_route_object > );
         break;
      }
      case( by_destination ):
      {
         auto key = args.start.as< std::pair< account_name_type, withdraw_vesting_route_id_type > >();
         iterate_results< chain::withdraw_vesting_route_index, chain::by_destination >(
            boost::make_tuple( key.first, key.second ),
            result.routes,
            args.limit,
            &database_api_impl::on_push_default< withdraw_vesting_route_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_withdraw_vesting_routes )
{
   find_withdraw_vesting_routes_return result;

   switch( args.order )
   {
      case( by_withdraw_route ):
      {
         const auto& route_idx = _db.get_index< chain::withdraw_vesting_route_index, chain::by_withdraw_route >();
         auto itr = route_idx.lower_bound( args.account );

         while( itr != route_idx.end() && itr->from_account == args.account && result.routes.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
         {
            result.routes.push_back( *itr );
            ++itr;
         }

         break;
      }
      case( by_destination ):
      {
         const auto& route_idx = _db.get_index< chain::withdraw_vesting_route_index, chain::by_destination >();
         auto itr = route_idx.lower_bound( args.account );

         while( itr != route_idx.end() && itr->to_account == args.account && result.routes.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
         {
            result.routes.push_back( *itr );
            ++itr;
         }

         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


/* Savings Withdrawals */

DEFINE_API_IMPL( database_api_impl, list_savings_withdrawals )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_savings_withdrawals_return result;
   result.withdrawals.reserve( args.limit );

   switch( args.order )
   {
      case( by_from_id ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::savings_withdraw_index, chain::by_from_rid >(
            boost::make_tuple( key.first, key.second ),
            result.withdrawals,
            args.limit,
            [&]( const savings_withdraw_object& w ){ return api_savings_withdraw_object( w ); } );
         break;
      }
      case( by_complete_from_id ):
      {
         auto key = args.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_complete_from_id start requires 3 values. (time_point_sec, account_name_type, uint32_t)" );
         iterate_results< chain::savings_withdraw_index, chain::by_complete_from_rid >(
            boost::make_tuple( key[0].as< fc::time_point_sec >(), key[1].as< account_name_type >(), key[2].as< uint32_t >() ),
            result.withdrawals,
            args.limit,
            [&]( const savings_withdraw_object& w ){ return api_savings_withdraw_object( w ); } );
         break;
      }
      case( by_to_complete ):
      {
         auto key = args.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_to_complete start requires 3 values. (account_name_type, time_point_sec, savings_withdraw_id_type" );
         iterate_results< chain::savings_withdraw_index, chain::by_to_complete >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< fc::time_point_sec >(), key[2].as< savings_withdraw_id_type >() ),
            result.withdrawals,
            args.limit,
            [&]( const savings_withdraw_object& w ){ return api_savings_withdraw_object( w ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_savings_withdrawals )
{
   find_savings_withdrawals_return result;
   const auto& withdraw_idx = _db.get_index< chain::savings_withdraw_index, chain::by_from_rid >();
   auto itr = withdraw_idx.lower_bound( args.account );

   while( itr != withdraw_idx.end() && itr->from == args.account && result.withdrawals.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.withdrawals.push_back( api_savings_withdraw_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Vesting Delegations */

DEFINE_API_IMPL( database_api_impl, list_vesting_delegations )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_vesting_delegations_return result;
   result.delegations.reserve( args.limit );

   switch( args.order )
   {
      case( by_delegation ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::vesting_delegation_index, chain::by_delegation >(
            boost::make_tuple( key.first, key.second ),
            result.delegations,
            args.limit,
            &database_api_impl::on_push_default< api_vesting_delegation_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_vesting_delegations )
{
   find_vesting_delegations_return result;
   const auto& delegation_idx = _db.get_index< chain::vesting_delegation_index, chain::by_delegation >();
   auto itr = delegation_idx.lower_bound( args.account );

   while( itr != delegation_idx.end() && itr->delegator == args.account && result.delegations.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.delegations.push_back( api_vesting_delegation_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Vesting Delegation Expirations */

DEFINE_API_IMPL( database_api_impl, list_vesting_delegation_expirations )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_vesting_delegation_expirations_return result;
   result.delegations.reserve( args.limit );

   switch( args.order )
   {
      case( by_expiration ):
      {
         auto key = args.start.as< std::pair< time_point_sec, vesting_delegation_expiration_id_type > >();
         iterate_results< chain::vesting_delegation_expiration_index, chain::by_expiration >(
            boost::make_tuple( key.first, key.second ),
            result.delegations,
            args.limit,
            &database_api_impl::on_push_default< api_vesting_delegation_expiration_object > );
         break;
      }
      case( by_account_expiration ):
      {
         auto key = args.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_account_expiration start requires 3 values. (account_name_type, time_point_sec, vesting_delegation_expiration_id_type" );
         iterate_results< chain::vesting_delegation_expiration_index, chain::by_account_expiration >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< time_point_sec >(), key[2].as< vesting_delegation_expiration_id_type >() ),
            result.delegations,
            args.limit,
            &database_api_impl::on_push_default< api_vesting_delegation_expiration_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_vesting_delegation_expirations )
{
   find_vesting_delegation_expirations_return result;
   const auto& del_exp_idx = _db.get_index< chain::vesting_delegation_expiration_index, chain::by_account_expiration >();
   auto itr = del_exp_idx.lower_bound( args.account );

   while( itr != del_exp_idx.end() && itr->delegator == args.account && result.delegations.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.delegations.push_back( *itr );
      ++itr;
   }

   return result;
}


/* SBD Conversion Requests */

DEFINE_API_IMPL( database_api_impl, list_sbd_conversion_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_sbd_conversion_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_conversion_date ):
      {
         auto key = args.start.as< std::pair< time_point_sec, convert_request_id_type > >();
         iterate_results< chain::convert_request_index, chain::by_conversion_date >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< api_convert_request_object > );
         break;
      }
      case( by_account ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::convert_request_index, chain::by_owner >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< api_convert_request_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_sbd_conversion_requests )
{
   find_sbd_conversion_requests_return result;
   const auto& convert_idx = _db.get_index< chain::convert_request_index, chain::by_owner >();
   auto itr = convert_idx.lower_bound( args.account );

   while( itr != convert_idx.end() && itr->owner == args.account && result.requests.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.requests.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Decline Voting Rights Requests */

DEFINE_API_IMPL( database_api_impl, list_decline_voting_rights_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_decline_voting_rights_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_account ):
      {
         iterate_results< chain::decline_voting_rights_request_index, chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< api_decline_voting_rights_request_object > );
         break;
      }
      case( by_effective_date ):
      {
         auto key = args.start.as< std::pair< time_point_sec, account_name_type > >();
         iterate_results< chain::decline_voting_rights_request_index, chain::by_effective_date >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< api_decline_voting_rights_request_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_decline_voting_rights_requests )
{
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
   find_decline_voting_rights_requests_return result;

   for( auto& a : args.accounts )
   {
      auto request = _db.find< chain::decline_voting_rights_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( *request );
   }

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Comments                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

/* Comments */

DEFINE_API_IMPL( database_api_impl, list_comments )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_comments_return result;
   result.comments.reserve( args.limit );

   switch( args.order )
   {
      case( by_cashout_time ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_cashout_time start requires 3 values. (time_point_sec, account_name_type, string)" );

         auto author = key[1].as< account_name_type >();
         auto permlink = key[2].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< chain::comment_index, chain::by_cashout_time >(
            boost::make_tuple( key[0].as< fc::time_point_sec >(), comment_id ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_permlink ):
      {
         auto key = args.start.as< std::pair< account_name_type, string > >();
         iterate_results< chain::comment_index, chain::by_permlink >(
            boost::make_tuple( key.first, key.second ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_root ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_root start requires 4 values. (account_name_type, string, account_name_type, string)" );

         auto root_author = key[0].as< account_name_type >();
         auto root_permlink = key[1].as< string >();
         comment_id_type root_id;

         if( root_author != account_name_type() || root_permlink.size() )
         {
            auto root = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( root_author, root_permlink ) );
            FC_ASSERT( root != nullptr, "Could not find comment ${a}/${p}.", ("a", root_author)("p", root_permlink) );
            root_id = root->id;
         }

         auto child_author = key[2].as< account_name_type >();
         auto child_permlink = key[3].as< string >();
         comment_id_type child_id;

         if( child_author != account_name_type() || child_permlink.size() )
         {
            auto child = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( child_author, child_permlink ) );
            FC_ASSERT( child != nullptr, "Could not find comment ${a}/${p}.", ("a", child_author)("p", child_permlink) );
            child_id = child->id;
         }

         iterate_results< chain::comment_index, chain::by_root >(
            boost::make_tuple( root_id, child_id ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_parent ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_parent start requires 4 values. (account_name_type, string, account_name_type, string)" );

         auto child_author = key[2].as< account_name_type >();
         auto child_permlink = key[3].as< string >();
         comment_id_type child_id;

         if( child_author != account_name_type() || child_permlink.size() )
         {
            auto child = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( child_author, child_permlink ) );
            FC_ASSERT( child != nullptr, "Could not find comment ${a}/${p}.", ("a", child_author)("p", child_permlink) );
            child_id = child->id;
         }

         iterate_results< chain::comment_index, chain::by_parent >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< string >(), child_id ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
#ifndef IS_LOW_MEM
      case( by_last_update ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_last_update start requires 4 values. (account_name_type, time_point_sec, account_name_type, string)" );

         auto child_author = key[2].as< account_name_type >();
         auto child_permlink = key[3].as< string >();
         comment_id_type child_id;

         if( child_author != account_name_type() || child_permlink.size() )
         {
            auto child = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( child_author, child_permlink ) );
            FC_ASSERT( child != nullptr, "Could not find comment ${a}/${p}.", ("a", child_author)("p", child_permlink) );
            child_id = child->id;
         }

         iterate_results< chain::comment_index, chain::by_last_update >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< fc::time_point_sec >(), child_id ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_author_last_update ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_author_last_update start requires 4 values. (account_name_type, time_point_sec, account_name_type, string)" );

         auto author = key[2].as< account_name_type >();
         auto permlink = key[3].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< chain::comment_index, chain::by_last_update >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< fc::time_point_sec >(), comment_id ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
#endif
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_comments )
{
   FC_ASSERT( args.comments.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
   find_comments_return result;
   result.comments.reserve( args.comments.size() );

   for( auto& key: args.comments )
   {
      auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( key.first, key.second ) );

      if( comment != nullptr )
         result.comments.push_back( api_comment_object( *comment, _db ) );
   }

   return result;
}

//====================================================last_votes_misc====================================================

namespace last_votes_misc
{

   //====================================================votes_impl====================================================
   template< sort_order_type SORTORDERTYPE >
   void votes_impl( database_api_impl& _impl, vector< api_comment_vote_object >& c, size_t nr_args, uint32_t limit, vector< fc::variant >& key, fc::time_point_sec& timestamp, uint64_t weight )
   {
      if( SORTORDERTYPE == by_comment_voter )
         FC_ASSERT( key.size() == nr_args, "by_comment_voter start requires ${nr_args} values. (account_name_type, string, account_name_type)", ("nr_args", nr_args ) );
      else
         FC_ASSERT( key.size() == nr_args, "by_comment_voter start requires ${nr_args} values. (account_name_type, ${desc}account_name_type, string)", ("nr_args", nr_args )("desc",( nr_args == 4 )?"time_point_sec, ":"" ) );

      account_name_type voter;
      account_name_type author;
      string permlink;

      account_id_type voter_id;
      comment_id_type comment_id;

      if( SORTORDERTYPE == by_comment_voter )
      {
         author = key[0].as< account_name_type >();
         permlink = key[1].as< string >();
         voter = key[ 2 ].as< account_name_type >();
      }
      else
      {
         author = key[ nr_args - 2 ].as< account_name_type >();
         permlink = key[ nr_args - 1 ].as< string >();
         voter = key[0].as< account_name_type >();
      }

      if( voter != account_name_type() )
      {
         auto account = _impl._db.find< chain::account_object, chain::by_name >( voter );
         FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
         voter_id = account->id;
      }

      if( author != account_name_type() || permlink.size() )
      {
         auto comment = _impl._db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
         FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
         comment_id = comment->id;
      }

      if( SORTORDERTYPE == by_comment_voter )
      {
         _impl.iterate_results< chain::comment_vote_index, chain::by_comment_voter >(
         boost::make_tuple( comment_id, voter_id ),
         c,
         limit,
         [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _impl._db ); } );
      }
      else if( SORTORDERTYPE == by_voter_comment )
      {
         _impl.iterate_results< chain::comment_vote_index, chain::by_voter_comment >(
         boost::make_tuple( voter_id, comment_id ),
         c,
         limit,
         [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _impl._db ); } );
      }
   }

}//namespace last_votes_misc

//====================================================last_votes_misc====================================================

/* Votes */

DEFINE_API_IMPL( database_api_impl, list_votes )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   auto key = args.start.as< vector< fc::variant > >();

   list_votes_return result;
   result.votes.reserve( args.limit );

   switch( args.order )
   {
      case( by_comment_voter ):
      {
         static fc::time_point_sec t( -1 );
         last_votes_misc::votes_impl< by_comment_voter >( *this, result.votes, 3/*nr_args*/, args.limit, key, t, 0 );
         break;
      }
      case( by_voter_comment ):
      {
         static fc::time_point_sec t( -1 );
         last_votes_misc::votes_impl< by_voter_comment >( *this, result.votes, 3/*nr_args*/, args.limit, key, t, 0 );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_votes )
{
   find_votes_return result;

   auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( args.author, args.permlink ) );
   FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}", ("a", args.author)("p", args.permlink ) );

   const auto& vote_idx = _db.get_index< chain::comment_vote_index, chain::by_comment_voter >();
   auto itr = vote_idx.lower_bound( comment->id );

   while( itr != vote_idx.end() && itr->comment == comment->id && result.votes.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.votes.push_back( api_comment_vote_object( *itr, _db ) );
      ++itr;
   }

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Market                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

/* Limit Orders */

DEFINE_API_IMPL( database_api_impl, list_limit_orders )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_limit_orders_return result;
   result.orders.reserve( args.limit );

   switch( args.order )
   {
      case( by_price ):
      {
         auto key = args.start.as< std::pair< price, limit_order_id_type > >();
         iterate_results< chain::limit_order_index, chain::by_price >(
            boost::make_tuple( key.first, key.second ),
            result.orders,
            args.limit,
            &database_api_impl::on_push_default< api_limit_order_object > );
         break;
      }
      case( by_account ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::limit_order_index, chain::by_account >(
            boost::make_tuple( key.first, key.second ),
            result.orders,
            args.limit,
            &database_api_impl::on_push_default< api_limit_order_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_limit_orders )
{
   find_limit_orders_return result;
   const auto& order_idx = _db.get_index< chain::limit_order_index, chain::by_account >();
   auto itr = order_idx.lower_bound( args.account );

   while( itr != order_idx.end() && itr->seller == args.account && result.orders.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.orders.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Order Book */

DEFINE_API_IMPL( database_api_impl, get_order_book )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
   get_order_book_return result;

   auto max_sell = price::max( SBD_SYMBOL, STEEM_SYMBOL );
   auto max_buy = price::max( STEEM_SYMBOL, SBD_SYMBOL );

   const auto& limit_price_idx = _db.get_index< chain::limit_order_index >().indices().get< chain::by_price >();
   auto sell_itr = limit_price_idx.lower_bound( max_sell );
   auto buy_itr  = limit_price_idx.lower_bound( max_buy );
   auto end = limit_price_idx.end();

   while( sell_itr != end && sell_itr->sell_price.base.symbol == SBD_SYMBOL && result.bids.size() < args.limit )
   {
      auto itr = sell_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = 0.0;
      // cur.real_price  = (cur.order_price).to_real();
      cur.sbd = itr->for_sale;
      cur.steem = ( asset( itr->for_sale, SBD_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.bids.push_back( cur );
      ++sell_itr;
   }
   while( buy_itr != end && buy_itr->sell_price.base.symbol == STEEM_SYMBOL && result.asks.size() < args.limit )
   {
      auto itr = buy_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price = 0.0;
      // cur.real_price  = (~cur.order_price).to_real();
      cur.steem   = itr->for_sale;
      cur.sbd     = ( asset( itr->for_sale, STEEM_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.asks.push_back( cur );
      ++buy_itr;
   }

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / Validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, get_transaction_hex )
{
   return get_transaction_hex_return( { fc::to_hex( fc::raw::pack_to_vector( args.trx ) ) } );
}

DEFINE_API_IMPL( database_api_impl, get_required_signatures )
{
   get_required_signatures_return result;
   result.keys = args.trx.get_required_signatures( _db.get_chain_id(),
                                                   args.available_keys,
                                                   [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).active  ); },
                                                   [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner   ); },
                                                   [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting ); },
                                                   STEEM_MAX_SIG_CHECK_DEPTH,
                                                   _db.has_hardfork( STEEM_HARDFORK_0_20__1944 ) ? fc::ecc::canonical_signature_type::bip_0062 : fc::ecc::canonical_signature_type::fc_canonical );

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_potential_signatures )
{
   get_potential_signatures_return result;
   args.trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set< public_key_type >(),
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).active;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      STEEM_MAX_SIG_CHECK_DEPTH,
      _db.has_hardfork( STEEM_HARDFORK_0_20__1944 ) ? fc::ecc::canonical_signature_type::bip_0062 : fc::ecc::canonical_signature_type::fc_canonical
   );

   return result;
}

DEFINE_API_IMPL( database_api_impl, verify_authority )
{
   args.trx.verify_authority(_db.get_chain_id(),
                           [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).active  ); },
                           [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner   ); },
                           [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting ); },
                           STEEM_MAX_SIG_CHECK_DEPTH,
                           STEEM_MAX_AUTHORITY_MEMBERSHIP,
                           STEEM_MAX_SIG_CHECK_ACCOUNTS,
                           _db.has_hardfork( STEEM_HARDFORK_0_20__1944 ) ? fc::ecc::canonical_signature_type::bip_0062 : fc::ecc::canonical_signature_type::fc_canonical );
   return verify_authority_return( { true } );
}

// TODO: This is broken. By the look of is, it has been since BitShares. verify_authority always
// returns false because the TX is not signed.
DEFINE_API_IMPL( database_api_impl, verify_account_authority )
{
   auto account = _db.find< chain::account_object, chain::by_name >( args.account );
   FC_ASSERT( account != nullptr, "no such account" );

   /// reuse trx.verify_authority by creating a dummy transfer
   verify_authority_args vap;
   transfer_operation op;
   op.from = account->name;
   vap.trx.operations.emplace_back( op );

   return verify_authority( vap );
}

DEFINE_API_IMPL( database_api_impl, verify_signatures )
{
   // get_signature_keys can throw for dup sigs. Allow this to throw.
   flat_set< public_key_type > sig_keys;
   for( const auto&  sig : args.signatures )
   {
      STEEM_ASSERT(
         sig_keys.insert( fc::ecc::public_key( sig, args.hash ) ).second,
         protocol::tx_duplicate_sig,
         "Duplicate Signature detected" );
   }

   verify_signatures_return result;
   result.valid = true;

   // verify authority throws on failure, catch and return false
   try
   {
      steem::protocol::verify_authority< verify_signatures_args >(
         { args },
         sig_keys,
         [this]( const string& name ) { return authority( _db.get< chain::account_authority_object, chain::by_account >( name ).owner ); },
         [this]( const string& name ) { return authority( _db.get< chain::account_authority_object, chain::by_account >( name ).active ); },
         [this]( const string& name ) { return authority( _db.get< chain::account_authority_object, chain::by_account >( name ).posting ); },
         STEEM_MAX_SIG_CHECK_DEPTH );
   }
   catch( fc::exception& ) { result.valid = false; }

   return result;
}

#ifdef STEEM_ENABLE_SMT
//////////////////////////////////////////////////////////////////////
//                                                                  //
// SMT                                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, get_nai_pool )
{
   get_nai_pool_return result;
   result.nai_pool = _db.get< nai_pool_object >().pool();
   return result;
}

DEFINE_API_IMPL( database_api_impl, list_smt_tokens )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_smt_tokens_return result;
   result.tokens.reserve( args.limit );

   switch( args.order )
   {
      case( by_symbol ):
      {
         asset_symbol_type start;

         if( args.start.get_object().size() > 0 )
         {
            start = args.start.as< asset_symbol_type >();
         }

         iterate_results< chain::smt_token_index, chain::by_symbol >(
            start,
            result.tokens,
            args.limit,
            &database_api_impl::on_push_default< chain::smt_token_object > );

         break;
      }
      case( by_control_account ):
      {
         boost::tuple< account_name_type, asset_symbol_type > start;

         if( args.start.is_string() )
         {
            start = boost::make_tuple( args.start.as< account_name_type >(), asset_symbol_type() );
         }
         else
         {
            auto key = args.start.get_array();
            FC_ASSERT( key.size() == 2, "The parameter 'start' must be an account name or an array containing an account name and an asset symbol" );

            start = boost::make_tuple( key[0].as< account_name_type >(), key[1].as< asset_symbol_type >() );
         }

         iterate_results< chain::smt_token_index, chain::by_control_account >(
            start,
            result.tokens,
            args.limit,
            &database_api_impl::on_push_default< chain::smt_token_object > );

         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_smt_tokens )
{
   FC_ASSERT( args.symbols.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   find_smt_tokens_return result;
   result.tokens.reserve( args.symbols.size() );

   for( auto& symbol : args.symbols )
   {
      const auto token = chain::util::smt::find_token( _db, symbol, args.ignore_precision );
      if( token != nullptr )
      {
         result.tokens.push_back( *token );
      }
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, list_smt_token_emissions )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_smt_token_emissions_return result;
   result.token_emissions.reserve( args.limit );

   switch( args.order )
   {
      case( by_symbol_time ):
      {
         auto key = args.start.get_array();
         FC_ASSERT( key.size() == 0 || key.size() == 2, "The parameter 'start' must be an empty array or consist of asset_symbol_type and time_point_sec" );

         boost::tuple< asset_symbol_type, time_point_sec > start;
         if ( key.size() == 0 )
            start = boost::make_tuple( asset_symbol_type(), time_point_sec() );
         else
            start = boost::make_tuple( key[ 0 ].as< asset_symbol_type >(), key[ 1 ].as< time_point_sec >() );

         iterate_results< chain::smt_token_emissions_index, chain::by_symbol_time >(
            start,
            result.token_emissions,
            args.limit,
            &database_api_impl::on_push_default< chain::smt_token_emissions_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_smt_token_emissions )
{
   find_smt_token_emissions_return result;

   const auto& idx = _db.get_index< chain::smt_token_emissions_index, chain::by_symbol_time >();
   auto itr = idx.lower_bound( args.asset_symbol );

   while( itr != idx.end() && itr->symbol == args.asset_symbol && result.token_emissions.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.token_emissions.push_back( *itr );
      ++itr;
   }

   return result;
}

#endif

DEFINE_LOCKLESS_APIS( database_api, (get_config)(get_version) )

DEFINE_READ_APIS( database_api,
   (get_dynamic_global_properties)
   (get_witness_schedule)
   (get_hardfork_properties)
   (get_reward_funds)
   (get_current_price_feed)
   (get_feed_history)
   (list_witnesses)
   (find_witnesses)
   (list_witness_votes)
   (get_active_witnesses)
   (list_accounts)
   (find_accounts)
   (list_owner_histories)
   (find_owner_histories)
   (list_account_recovery_requests)
   (find_account_recovery_requests)
   (list_change_recovery_account_requests)
   (find_change_recovery_account_requests)
   (list_escrows)
   (find_escrows)
   (list_withdraw_vesting_routes)
   (find_withdraw_vesting_routes)
   (list_savings_withdrawals)
   (find_savings_withdrawals)
   (list_vesting_delegations)
   (find_vesting_delegations)
   (list_vesting_delegation_expirations)
   (find_vesting_delegation_expirations)
   (list_sbd_conversion_requests)
   (find_sbd_conversion_requests)
   (list_decline_voting_rights_requests)
   (find_decline_voting_rights_requests)
   (list_comments)
   (find_comments)
   (list_votes)
   (find_votes)
   (list_limit_orders)
   (find_limit_orders)
   (get_order_book)
   (get_transaction_hex)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)
   (verify_signatures)
#ifdef STEEM_ENABLE_SMT
   (get_nai_pool)
   (list_smt_tokens)
   (find_smt_tokens)
   (list_smt_token_emissions)
   (find_smt_token_emissions)
#endif
)

} } } // steem::plugins::database_api
