#include <appbase/application.hpp>

#include <steemit/plugins/database_api/database_api.hpp>
#include <steemit/plugins/database_api/database_api_plugin.hpp>

#include <steemit/protocol/get_config.hpp>

namespace steemit { namespace plugins { namespace database_api {

class database_api_impl
{
   public:
      database_api_impl();
      ~database_api_impl();

      DECLARE_API
      (
         (get_block_header)
         (get_block)
         (get_config)
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

      steemit::chain::database& _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api()
   : my( new database_api_impl() )
{
   JSON_RPC_REGISTER_API(
      STEEM_DATABASE_API_PLUGIN_NAME,
      (get_block_header)
      (get_block)
      (get_config)
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
   );
}

database_api::~database_api() {}

database_api_impl::database_api_impl()
   : _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ) {}

database_api_impl::~database_api_impl() {}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API( database_api, get_block_header )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block_header( args );
   });
}

DEFINE_API( database_api_impl, get_block_header )
{
   get_block_header_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.header = *block;

   return result;
}


DEFINE_API( database_api, get_block )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block( args );
   });
}

DEFINE_API( database_api_impl, get_block )
{
   get_block_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.block = *block;

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API( database_api, get_config )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_config( args );
   });
}

DEFINE_API( database_api_impl, get_config )
{
   return steemit::protocol::get_config();
}


DEFINE_API( database_api, get_dynamic_global_properties )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_dynamic_global_properties( args );
   });
}

DEFINE_API( database_api_impl, get_dynamic_global_properties )
{
   return _db.get_dynamic_global_properties();
}


DEFINE_API( database_api, get_witness_schedule )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_witness_schedule( args );
   });
}

DEFINE_API( database_api_impl, get_witness_schedule )
{
   return api_witness_schedule_object( _db.get_witness_schedule_object() );
}


DEFINE_API( database_api, get_hardfork_properties )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_hardfork_properties( args );
   });
}

DEFINE_API( database_api_impl, get_hardfork_properties )
{
   return _db.get_hardfork_property_object();
}


DEFINE_API( database_api, get_reward_funds )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_reward_funds( args );
   });
}

DEFINE_API( database_api_impl, get_reward_funds )
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


DEFINE_API( database_api, get_current_price_feed )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_current_price_feed( args );
   });
}

DEFINE_API( database_api_impl, get_current_price_feed )
{
   return _db.get_feed_history().current_median_history;;
}


DEFINE_API( database_api, get_feed_history )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_feed_history( args );
   });
}

DEFINE_API( database_api_impl, get_feed_history )
{
   return _db.get_feed_history();
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API( database_api, list_witnesses )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_witnesses( args );
   });
}

DEFINE_API( database_api_impl, list_witnesses )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_witnesses_return result;
   result.witnesses.reserve( args.limit );

   switch( args.order )
   {
      case( by_name ):
      {
         iterate_results< steemit::chain::witness_index, steemit::chain::by_name >(
            args.start.as< protocol::account_name_type >(),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      case( by_vote_name ):
      {
         auto key = args.start.as< std::pair< share_type, account_name_type > >();
         iterate_results< steemit::chain::witness_index, steemit::chain::by_vote_name >(
            boost::make_tuple( key.first, key.second ),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      case( by_schedule_time ):
      {
         auto key = args.start.as< std::pair< fc::uint128, account_name_type > >();
         auto wit_id = _db.get< steemit::chain::witness_object, steemit::chain::by_name >( key.second ).id;
         iterate_results< steemit::chain::witness_index, steemit::chain::by_schedule_time >(
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


DEFINE_API( database_api, find_witnesses )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_witnesses( args );
   });
}

DEFINE_API( database_api_impl, find_witnesses )
{
   FC_ASSERT( args.owners.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   find_witnesses_return result;

   for( auto& o : args.owners )
   {
      auto witness = _db.find< steemit::chain::witness_object, steemit::chain::by_name >( o );

      if( witness != nullptr )
         result.witnesses.push_back( api_witness_object( *witness ) );
   }

   return result;
}


DEFINE_API( database_api, list_witness_votes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_witness_votes( args );
   });
}

DEFINE_API( database_api_impl, list_witness_votes )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_witness_votes_return result;
   result.votes.reserve( args.limit );

   switch( args.order )
   {
      case( by_account_witness ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< steemit::chain::witness_vote_index, steemit::chain::by_account_witness >(
            boost::make_tuple( key.first, key.second ),
            result.votes,
            args.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); } );
         break;
      }
      case( by_witness_account ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< steemit::chain::witness_vote_index, steemit::chain::by_witness_account >(
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


DEFINE_API( database_api, get_active_witnesses )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_active_witnesses( args );
   });
}

DEFINE_API( database_api_impl, get_active_witnesses )
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

DEFINE_API( database_api, list_accounts )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_accounts( args );
   });
}

DEFINE_API( database_api_impl, list_accounts )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_accounts_return result;
   result.accounts.reserve( args.limit );

   switch( args.order )
   {
      case( by_name ):
      {
         iterate_results< steemit::chain::account_index, steemit::chain::by_name >(
            args.start.as< protocol::account_name_type >(),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_proxy ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< steemit::chain::account_index, steemit::chain::by_proxy >(
            boost::make_tuple( key.first, key.second ),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_next_vesting_withdrawal ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< steemit::chain::account_index, steemit::chain::by_next_vesting_withdrawal >(
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


DEFINE_API( database_api, find_accounts )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_accounts( args );
   });
}

DEFINE_API( database_api_impl, find_accounts )
{
   find_accounts_return result;
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto acct = _db.find< steemit::chain::account_object, steemit::chain::by_name >( a );
      if( acct != nullptr )
         result.accounts.push_back( api_account_object( *acct, _db ) );
   }

   return result;
}


/* Owner Auth Histories */

DEFINE_API( database_api, list_owner_histories )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_owner_histories( args );
   });
}

DEFINE_API( database_api_impl, list_owner_histories )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_owner_histories_return result;
   result.owner_auths.reserve( args.limit );

   auto key = args.start.as< std::pair< account_name_type, fc::time_point_sec > >();
   iterate_results< steemit::chain::owner_authority_history_index, steemit::chain::by_account >(
      boost::make_tuple( key.first, key.second ),
      result.owner_auths,
      args.limit,
      [&]( const owner_authority_history_object& o ){ return api_owner_authority_history_object( o ); } );

   return result;
}


DEFINE_API( database_api, find_owner_histories )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_owner_histories( args );
   });
}

DEFINE_API( database_api_impl, find_owner_histories )
{
   find_owner_histories_return result;

   const auto& hist_idx = _db.get_index< steemit::chain::owner_authority_history_index, steemit::chain::by_account >();
   auto itr = hist_idx.lower_bound( args.owner );

   while( itr != hist_idx.end() && itr->account == args.owner && result.owner_auths.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.owner_auths.push_back( api_owner_authority_history_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Account Recovery Requests */

DEFINE_API( database_api, list_account_recovery_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_account_recovery_requests( args );
   });
}

DEFINE_API( database_api_impl, list_account_recovery_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_account_recovery_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_account ):
      {
         iterate_results< steemit::chain::account_recovery_request_index, steemit::chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); } );
         break;
      }
      case( by_expiration ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< steemit::chain::account_recovery_request_index, steemit::chain::by_expiration >(
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


DEFINE_API( database_api, find_account_recovery_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_account_recovery_requests( args );
   });
}

DEFINE_API( database_api_impl, find_account_recovery_requests )
{
   find_account_recovery_requests_return result;
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto request = _db.find< steemit::chain::account_recovery_request_object, steemit::chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( api_account_recovery_request_object( *request ) );
   }

   return result;
}


/* Change Recovery Account Requests */

DEFINE_API( database_api, list_change_recovery_account_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_change_recovery_account_requests( args );
   });
}

DEFINE_API( database_api_impl, list_change_recovery_account_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_change_recovery_account_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_account ):
      {
         iterate_results< steemit::chain::change_recovery_account_request_index, steemit::chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< change_recovery_account_request_object > );
         break;
      }
      case( by_effective_date ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< steemit::chain::change_recovery_account_request_index, steemit::chain::by_effective_date >(
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


DEFINE_API( database_api, find_change_recovery_account_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_change_recovery_account_requests( args );
   });
}

DEFINE_API( database_api_impl, find_change_recovery_account_requests )
{
   find_change_recovery_account_requests_return result;
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto request = _db.find< steemit::chain::change_recovery_account_request_object, steemit::chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( *request );
   }

   return result;
}


/* Escrows */

DEFINE_API( database_api, list_escrows )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_escrows( args );
   });
}

DEFINE_API( database_api_impl, list_escrows )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_escrows_return result;
   result.escrows.reserve( args.limit );

   switch( args.order )
   {
      case( by_from_id ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< steemit::chain::escrow_index, steemit::chain::by_from_id >(
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
         iterate_results< steemit::chain::escrow_index, steemit::chain::by_ratification_deadline >(
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


DEFINE_API( database_api, find_escrows )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_escrows( args );
   });
}

DEFINE_API( database_api_impl, find_escrows )
{
   find_escrows_return result;

   const auto& escrow_idx = _db.get_index< steemit::chain::escrow_index, steemit::chain::by_from_id >();
   auto itr = escrow_idx.lower_bound( args.from );

   while( itr != escrow_idx.end() && itr->from == args.from && result.escrows.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.escrows.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Withdraw Vesting Routes */

DEFINE_API( database_api, list_withdraw_vesting_routes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_withdraw_vesting_routes( args );
   });
}

DEFINE_API( database_api_impl, list_withdraw_vesting_routes )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_withdraw_vesting_routes_return result;
   result.routes.reserve( args.limit );

   switch( args.order )
   {
      case( by_withdraw_route ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< steemit::chain::withdraw_vesting_route_index, steemit::chain::by_withdraw_route >(
            boost::make_tuple( key.first, key.second ),
            result.routes,
            args.limit,
            &database_api_impl::on_push_default< withdraw_vesting_route_object > );
         break;
      }
      case( by_destination ):
      {
         auto key = args.start.as< std::pair< account_name_type, withdraw_vesting_route_id_type > >();
         iterate_results< steemit::chain::withdraw_vesting_route_index, steemit::chain::by_destination >(
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


DEFINE_API( database_api, find_withdraw_vesting_routes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_withdraw_vesting_routes( args );
   });
}

DEFINE_API( database_api_impl, find_withdraw_vesting_routes )
{
   find_withdraw_vesting_routes_return result;

   switch( args.order )
   {
      case( by_withdraw_route ):
      {
         const auto& route_idx = _db.get_index< steemit::chain::withdraw_vesting_route_index, steemit::chain::by_withdraw_route >();
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
         const auto& route_idx = _db.get_index< steemit::chain::withdraw_vesting_route_index, steemit::chain::by_destination >();
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

DEFINE_API( database_api, list_savings_withdrawals )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_savings_withdrawals( args );
   });
}

DEFINE_API( database_api_impl, list_savings_withdrawals )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_savings_withdrawals_return result;
   result.withdrawals.reserve( args.limit );

   switch( args.order )
   {
      case( by_from_id ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< steemit::chain::savings_withdraw_index, steemit::chain::by_from_rid >(
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
         iterate_results< steemit::chain::savings_withdraw_index, steemit::chain::by_complete_from_rid >(
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
         iterate_results< steemit::chain::savings_withdraw_index, steemit::chain::by_to_complete >(
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


DEFINE_API( database_api, find_savings_withdrawals )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_savings_withdrawals( args );
   });
}

DEFINE_API( database_api_impl, find_savings_withdrawals )
{
   find_savings_withdrawals_return result;
   const auto& withdraw_idx = _db.get_index< steemit::chain::savings_withdraw_index, steemit::chain::by_from_rid >();
   auto itr = withdraw_idx.lower_bound( args.account );

   while( itr != withdraw_idx.end() && itr->from == args.account && result.withdrawals.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.withdrawals.push_back( api_savings_withdraw_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Vesting Delegations */

DEFINE_API( database_api, list_vesting_delegations )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_vesting_delegations( args );
   });
}

DEFINE_API( database_api_impl, list_vesting_delegations )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_vesting_delegations_return result;
   result.delegations.reserve( args.limit );

   switch( args.order )
   {
      case( by_delegation ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< steemit::chain::vesting_delegation_index, steemit::chain::by_delegation >(
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

DEFINE_API( database_api, find_vesting_delegations )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_vesting_delegations( args );
   });
}

DEFINE_API( database_api_impl, find_vesting_delegations )
{
   find_vesting_delegations_return result;
   const auto& delegation_idx = _db.get_index< steemit::chain::vesting_delegation_index, steemit::chain::by_delegation >();
   auto itr = delegation_idx.lower_bound( args.account );

   while( itr != delegation_idx.end() && itr->delegator == args.account && result.delegations.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.delegations.push_back( api_vesting_delegation_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Vesting Delegation Expirations */

DEFINE_API( database_api, list_vesting_delegation_expirations )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_vesting_delegation_expirations( args );
   });
}

DEFINE_API( database_api_impl, list_vesting_delegation_expirations )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_vesting_delegation_expirations_return result;
   result.delegations.reserve( args.limit );

   switch( args.order )
   {
      case( by_expiration ):
      {
         auto key = args.start.as< std::pair< time_point_sec, vesting_delegation_expiration_id_type > >();
         iterate_results< steemit::chain::vesting_delegation_expiration_index, steemit::chain::by_expiration >(
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
         iterate_results< steemit::chain::vesting_delegation_expiration_index, steemit::chain::by_account_expiration >(
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


DEFINE_API( database_api, find_vesting_delegation_expirations )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_vesting_delegation_expirations( args );
   });
}

DEFINE_API( database_api_impl, find_vesting_delegation_expirations )
{
   find_vesting_delegation_expirations_return result;
   const auto& del_exp_idx = _db.get_index< steemit::chain::vesting_delegation_expiration_index, steemit::chain::by_account_expiration >();
   auto itr = del_exp_idx.lower_bound( args.account );

   while( itr != del_exp_idx.end() && itr->delegator == args.account && result.delegations.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.delegations.push_back( *itr );
      ++itr;
   }

   return result;
}


/* SBD Conversion Requests */

DEFINE_API( database_api, list_sbd_conversion_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_sbd_conversion_requests( args );
   });
}

DEFINE_API( database_api_impl, list_sbd_conversion_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_sbd_conversion_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_conversion_date ):
      {
         auto key = args.start.as< std::pair< time_point_sec, convert_request_id_type > >();
         iterate_results< steemit::chain::convert_request_index, steemit::chain::by_conversion_date >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< api_convert_request_object > );
         break;
      }
      case( by_account ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< steemit::chain::convert_request_index, steemit::chain::by_owner >(
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


DEFINE_API( database_api, find_sbd_conversion_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_sbd_conversion_requests( args );
   });
}

DEFINE_API( database_api_impl, find_sbd_conversion_requests )
{
   find_sbd_conversion_requests_return result;
   const auto& convert_idx = _db.get_index< steemit::chain::convert_request_index, steemit::chain::by_owner >();
   auto itr = convert_idx.lower_bound( args.account );

   while( itr != convert_idx.end() && itr->owner == args.account && result.requests.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.requests.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Decline Voting Rights Requests */

DEFINE_API( database_api, list_decline_voting_rights_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_decline_voting_rights_requests( args );
   });
}

DEFINE_API( database_api_impl, list_decline_voting_rights_requests )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_decline_voting_rights_requests_return result;
   result.requests.reserve( args.limit );

   switch( args.order )
   {
      case( by_account ):
      {
         iterate_results< steemit::chain::decline_voting_rights_request_index, steemit::chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< api_decline_voting_rights_request_object > );
         break;
      }
      case( by_effective_date ):
      {
         auto key = args.start.as< std::pair< time_point_sec, account_name_type > >();
         iterate_results< steemit::chain::decline_voting_rights_request_index, steemit::chain::by_effective_date >(
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


DEFINE_API( database_api, find_decline_voting_rights_requests )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_decline_voting_rights_requests( args );
   });
}

DEFINE_API( database_api_impl, find_decline_voting_rights_requests )
{
   FC_ASSERT( args.accounts.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
   find_decline_voting_rights_requests_return result;

   for( auto& a : args.accounts )
   {
      auto request = _db.find< steemit::chain::decline_voting_rights_request_object, steemit::chain::by_account >( a );

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

DEFINE_API( database_api, list_comments )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_comments( args );
   });
}

DEFINE_API( database_api_impl, list_comments )
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
            auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< steemit::chain::comment_index, steemit::chain::by_cashout_time >(
            boost::make_tuple( key[0].as< fc::time_point_sec >(), comment_id ),
            result.comments,
            args.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_permlink ):
      {
         auto key = args.start.as< std::pair< account_name_type, string > >();
         iterate_results< steemit::chain::comment_index, steemit::chain::by_permlink >(
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
            auto root = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( root_author, root_permlink ) );
            FC_ASSERT( root != nullptr, "Could not find comment ${a}/${p}.", ("a", root_author)("p", root_permlink) );
            root_id = root->id;
         }

         auto child_author = key[2].as< account_name_type >();
         auto child_permlink = key[3].as< string >();
         comment_id_type child_id;

         if( child_author != account_name_type() || child_permlink.size() )
         {
            auto child = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( child_author, child_permlink ) );
            FC_ASSERT( child != nullptr, "Could not find comment ${a}/${p}.", ("a", child_author)("p", child_permlink) );
            child_id = child->id;
         }

         iterate_results< steemit::chain::comment_index, steemit::chain::by_root >(
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
            auto child = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( child_author, child_permlink ) );
            FC_ASSERT( child != nullptr, "Could not find comment ${a}/${p}.", ("a", child_author)("p", child_permlink) );
            child_id = child->id;
         }

         iterate_results< steemit::chain::comment_index, steemit::chain::by_parent >(
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
            auto child = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( child_author, child_permlink ) );
            FC_ASSERT( child != nullptr, "Could not find comment ${a}/${p}.", ("a", child_author)("p", child_permlink) );
            child_id = child->id;
         }

         iterate_results< steemit::chain::comment_index, steemit::chain::by_last_update >(
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
            auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< steemit::chain::comment_index, steemit::chain::by_last_update >(
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


DEFINE_API( database_api, find_comments )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_comments( args );
   });
}

DEFINE_API( database_api_impl, find_comments )
{
   FC_ASSERT( args.comments.size() <= DATABASE_API_SINGLE_QUERY_LIMIT );
   find_comments_return result;
   result.comments.reserve( args.comments.size() );

   for( auto& key: args.comments )
   {
      auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( key.first, key.second ) );

      if( comment != nullptr )
         result.comments.push_back( api_comment_object( *comment, _db ) );
   }

   return result;
}


/* Votes */

DEFINE_API( database_api, list_votes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_votes( args );
   });
}

DEFINE_API( database_api_impl, list_votes )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_votes_return result;
   result.votes.reserve( args.limit );

   switch( args.order )
   {
      case( by_comment_voter ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_comment_voter start requires 3 values. (account_name_type, string, account_name_type)" );

         auto author = key[0].as< account_name_type >();
         auto permlink = key[1].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         auto voter = key[2].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< steemit::chain::account_object, steemit::chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         iterate_results< steemit::chain::comment_vote_index, steemit::chain::by_comment_voter >(
            boost::make_tuple( comment_id, voter_id ),
            result.votes,
            args.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      case( by_voter_comment ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_comment_voter start requires 3 values. (account_name_type, account_name_type, string)" );

         auto voter = key[1].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< steemit::chain::account_object, steemit::chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         auto author = key[1].as< account_name_type >();
         auto permlink = key[2].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< steemit::chain::comment_vote_index, steemit::chain::by_voter_comment >(
            boost::make_tuple( voter_id, comment_id ),
            result.votes,
            args.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      case( by_voter_last_update ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_comment_voter start requires 4 values. (account_name_type, time_point_sec, account_name_type, string)" );

         auto voter = key[1].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< steemit::chain::account_object, steemit::chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         auto author = key[2].as< account_name_type >();
         auto permlink = key[3].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< steemit::chain::comment_vote_index, steemit::chain::by_voter_last_update >(
            boost::make_tuple( voter_id, key[1].as< fc::time_point_sec >(), comment_id ),
            result.votes,
            args.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      case( by_comment_weight_voter ):
      {
         auto key = args.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_comment_voter start requires 4 values. (account_name_type, string, uint64_t, account_name_type)" );

         auto author = key[0].as< account_name_type >();
         auto permlink = key[1].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         auto voter = key[3].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< steemit::chain::account_object, steemit::chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         iterate_results< steemit::chain::comment_vote_index, steemit::chain::by_comment_weight_voter >(
            boost::make_tuple( comment_id, key[2].as< uint64_t >(), voter_id ),
            result.votes,
            args.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


DEFINE_API( database_api, find_votes )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_votes( args );
   });
}

DEFINE_API( database_api_impl, find_votes )
{
   find_votes_return result;

   auto comment = _db.find< steemit::chain::comment_object, steemit::chain::by_permlink >( boost::make_tuple( args.author, args.permlink ) );
   FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}", ("a", args.author)("p", args.permlink ) );

   const auto& vote_idx = _db.get_index< steemit::chain::comment_vote_index, steemit::chain::by_comment_voter >();
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

DEFINE_API( database_api, list_limit_orders )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_limit_orders( args );
   });
}

DEFINE_API( database_api_impl, list_limit_orders )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );

   list_limit_orders_return result;
   result.orders.reserve( args.limit );

   switch( args.order )
   {
      case( by_price ):
      {
         auto key = args.start.as< std::pair< price, limit_order_id_type > >();
         iterate_results< steemit::chain::limit_order_index, steemit::chain::by_price >(
            boost::make_tuple( key.first, key.second ),
            result.orders,
            args.limit,
            &database_api_impl::on_push_default< api_limit_order_object > );
         break;
      }
      case( by_account ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< steemit::chain::limit_order_index, steemit::chain::by_account >(
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


DEFINE_API( database_api, find_limit_orders )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_limit_orders( args );
   });
}

DEFINE_API( database_api_impl, find_limit_orders )
{
   find_limit_orders_return result;
   const auto& order_idx = _db.get_index< steemit::chain::limit_order_index, steemit::chain::by_account >();
   auto itr = order_idx.lower_bound( args.account );

   while( itr != order_idx.end() && itr->seller == args.account && result.orders.size() <= DATABASE_API_SINGLE_QUERY_LIMIT )
   {
      result.orders.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Order Book */

DEFINE_API( database_api, get_order_book )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_order_book( args );
   });
}

DEFINE_API( database_api_impl, get_order_book )
{
   FC_ASSERT( args.limit <= DATABASE_API_SINGLE_QUERY_LIMIT );
   get_order_book_return result;

   auto max_sell = price::max( SBD_SYMBOL, STEEM_SYMBOL );
   auto max_buy = price::max( STEEM_SYMBOL, SBD_SYMBOL );

   const auto& limit_price_idx = _db.get_index< steemit::chain::limit_order_index >().indices().get< steemit::chain::by_price >();
   auto sell_itr = limit_price_idx.lower_bound( max_sell );
   auto buy_itr  = limit_price_idx.lower_bound( max_buy );
   auto end = limit_price_idx.end();

   while( sell_itr != end && sell_itr->sell_price.base.symbol == SBD_SYMBOL && result.bids.size() < args.limit )
   {
      auto itr = sell_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (cur.order_price).to_real();
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
      cur.real_price  = (~cur.order_price).to_real();
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

DEFINE_API( database_api, get_transaction_hex )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_transaction_hex( args );
   });
}

DEFINE_API( database_api_impl, get_transaction_hex )
{
   return get_transaction_hex_return( { fc::to_hex( fc::raw::pack( args.trx ) ) } );
}



DEFINE_API( database_api, get_required_signatures )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_required_signatures( args );
   });
}

DEFINE_API( database_api_impl, get_required_signatures )
{
   get_required_signatures_return result;
   result.keys = args.trx.get_required_signatures( STEEMIT_CHAIN_ID,
                                                   args.available_keys,
                                                   [&]( string account_name ){ return authority( _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).active  ); },
                                                   [&]( string account_name ){ return authority( _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).owner   ); },
                                                   [&]( string account_name ){ return authority( _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).posting ); },
                                                   STEEMIT_MAX_SIG_CHECK_DEPTH );

   return result;
}


DEFINE_API( database_api, get_potential_signatures )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_potential_signatures( args );
   });
}

DEFINE_API( database_api_impl, get_potential_signatures )
{
   get_potential_signatures_return result;
   args.trx.get_required_signatures(
      STEEMIT_CHAIN_ID,
      flat_set< public_key_type >(),
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).active;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).owner;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).posting;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      STEEMIT_MAX_SIG_CHECK_DEPTH
   );

   return result;
}


DEFINE_API( database_api, verify_authority )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_authority( args );
   });
}

DEFINE_API( database_api_impl, verify_authority )
{
   args.trx.verify_authority( STEEMIT_CHAIN_ID,
                           [&]( string account_name ){ return authority( _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).active  ); },
                           [&]( string account_name ){ return authority( _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).owner   ); },
                           [&]( string account_name ){ return authority( _db.get< steemit::chain::account_authority_object, steemit::chain::by_account >( account_name ).posting ); },
                           STEEMIT_MAX_SIG_CHECK_DEPTH );
   return verify_authority_return( { true } );
}


DEFINE_API( database_api, verify_account_authority )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_account_authority( args );
   });
}

// TODO: This is broken. By the look of is, it has been since BitShares. verify_authority always
// returns false because the TX is not signed.
DEFINE_API( database_api_impl, verify_account_authority )
{
   auto account = _db.find< steemit::chain::account_object, steemit::chain::by_name >( args.account );
   FC_ASSERT( account != nullptr, "no such account" );

   /// reuse trx.verify_authority by creating a dummy transfer
   verify_authority_args vap;
   transfer_operation op;
   op.from = account->name;
   vap.trx.operations.emplace_back( op );

   return verify_authority( vap );
}

} } } // steemit::plugins::database_api
