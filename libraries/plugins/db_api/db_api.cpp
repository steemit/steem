#include <steemit/db_api/db_api.hpp>

#include <steemit/protocol/get_config.hpp>

namespace steemit { namespace db_api {

class db_api_impl : public std::enable_shared_from_this< db_api_impl >
{
   public:
      db_api_impl( const steemit::app::api_context& ctx );
      ~db_api_impl();


      // Blocks and transactions
      optional< block_header > get_block_header( const get_block_header_params& p );
      optional< api_signed_block_object > get_block( const get_block_params& p );
      vector< api_operation_object > get_ops_in_block( const get_ops_in_block_params& p );

      // Globals
      fc::variant_object get_config();
      api_dynamic_global_property_object get_dynamic_global_properties();
      api_witness_schedule_object get_witness_schedule();
      api_hardfork_property_object get_hardfork_properties();
      vector< api_reward_fund_object > get_reward_funds();
      price get_current_price_feed();
      api_feed_history_object get_feed_history();


      // Witnesses
      vector< api_witness_object > list_witnesses( const list_witnesses_params& p );
      vector< api_witness_object > find_witnesses( const find_witnesses_params& p );

      vector< api_witness_vote_object > list_witness_votes( const list_witness_votes_params& p );

      vector< account_name_type > get_active_witnesses();


      // Accounts
      vector< api_account_object > list_accounts( const list_accounts_params& p );
      vector< api_account_object > find_accounts( const find_accounts_params& p );
      vector< api_owner_authority_history_object > list_owner_histories( const list_owner_histories_params& p );
      vector< api_owner_authority_history_object > find_owner_histories( const find_owner_histories_params& p );
      vector< api_account_recovery_request_object > list_account_recovery_requests( const list_account_recovery_requests_params& p );
      vector< api_account_recovery_request_object > find_account_recovery_requests( const find_account_recovery_requests_params& p );
      vector< api_change_recovery_account_request_object > list_change_recovery_account_requests( const list_change_recovery_account_requests_params& p );
      vector< api_change_recovery_account_request_object > find_change_recovery_account_requests( const find_change_recovery_account_requests_params& p );
      vector< api_escrow_object > list_escrows( const list_escrows_params& p );
      vector< api_escrow_object > find_escrows( const find_escrows_params& p );
      vector< api_withdraw_vesting_route_object > list_withdraw_vesting_routes( const list_withdraw_vesting_routes_params& p );
      vector< api_withdraw_vesting_route_object > find_withdraw_vesting_routes( const find_withdraw_vesting_routes_params& p );
      vector< api_savings_withdraw_object > list_savings_withdrawals( const list_savings_withdrawals_params& p );
      vector< api_savings_withdraw_object > find_savings_withdrawals( const find_savings_withdrawals_params& p );
      vector< api_vesting_delegation_object > list_vesting_delegations( const list_vesting_delegations_params& p );
      vector< api_vesting_delegation_object > find_vesting_delegations( const find_vesting_delegations_params& p );
      vector< api_vesting_delegation_expiration_object > list_vesting_delegation_expirations( const list_vesting_delegation_expirations_params& p );
      vector< api_vesting_delegation_expiration_object > find_vesting_delegation_expirations( const find_vesting_delegation_expirations_params& p );
      vector< api_convert_request_object > list_sbd_conversion_requests( const list_sbd_conversion_requests_params& p );
      vector< api_convert_request_object > find_sbd_conversion_requests( const find_sbd_conversion_requests_params& p );
      vector< api_decline_voting_rights_request_object > list_decline_voting_rights_requests( const list_decline_voting_rights_requests_params& p );
      vector< api_decline_voting_rights_request_object > find_decline_voting_rights_requests( const find_decline_voting_rights_requests_params& p );


      // Comments
      vector< api_comment_object > list_comments( const list_comments_params& p );
      vector< api_comment_object > find_comments( const find_comments_params& p );
      vector< api_comment_vote_object > list_votes( const list_votes_params& p );
      vector< api_comment_vote_object > find_votes( const find_votes_params& p );


      // Markets
      vector< api_limit_order_object > list_limit_orders( const list_limit_orders_params& p );
      vector< api_limit_order_object > find_limit_orders( const find_limit_orders_params& p );
      order_book get_order_book( const get_order_book_params& p );


      // Authority / validation
      std::string                   get_transaction_hex( const get_transaction_hex_params& p );
      annotated_signed_transaction  get_transaction( const get_transaction_params& p );
      set<public_key_type> get_required_signatures( const get_required_signatures_params& p );
      set<public_key_type> get_potential_signatures( const get_potential_signatures_params& p );
      bool           verify_authority( const verify_authority_params& p );
      bool           verify_account_authority( const verify_account_authority_params& p );

      template< typename ResultType >
      static ResultType on_push_default( const ResultType& r ) { return r; }

      template< typename IndexType, typename OrderType, typename ValueType, typename ResultType, typename OnPush >
      void iterate_results( ValueType start, vector< ResultType >& result, uint32_t limit, OnPush&& on_push = &db_api_impl::on_push_default< ResultType > )
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

db_api::db_api( const steemit::app::api_context& ctx )
   : my( new db_api_impl( ctx ) ) {}

db_api::~db_api() {}

db_api_impl::db_api_impl( const steemit::app::api_context& ctx )
   : _db( *ctx.app.chain_database() )
{
   wlog("creating db api ${x}", ("x",int64_t(this)) );
}

db_api_impl::~db_api_impl()
{
   wlog("freeing db api ${x}", ("x",int64_t(this)) );
}

void db_api::on_api_startup() {}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional< block_header > db_api::get_block_header( const get_block_header_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block_header( p );
   });
}

optional< block_header > db_api_impl::get_block_header( const get_block_header_params& p )
{
   auto result = _db.fetch_block_by_number( p.block_num );
   if(result)
      return *result;
   return {};
}


optional< api_signed_block_object > db_api::get_block( const get_block_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block( p );
   });
}

optional< api_signed_block_object > db_api_impl::get_block( const get_block_params& p )
{
   return _db.fetch_block_by_number( p.block_num );
}


vector< api_operation_object > db_api::get_ops_in_block( const get_ops_in_block_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_ops_in_block( p );
   });
}

vector< api_operation_object > db_api_impl::get_ops_in_block( const get_ops_in_block_params& p )
{
   const auto& idx = _db.get_index< operation_index >().indices().get< by_location >();
   auto itr = idx.lower_bound( p.block_num );
   vector< api_operation_object > result;
   api_operation_object temp;
   while( itr != idx.end() && itr->block == p.block_num )
   {
      temp = *itr;
      if( !p.only_virtual || is_virtual_operation( temp.op ) )
         result.push_back( temp );
      ++itr;
   }
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variant_object db_api::get_config()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_config();
   });
}

fc::variant_object db_api_impl::get_config()
{
   return steemit::protocol::get_config();
}


api_dynamic_global_property_object db_api::get_dynamic_global_properties()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_dynamic_global_properties();
   });
}

api_dynamic_global_property_object db_api_impl::get_dynamic_global_properties()
{
   return _db.get_dynamic_global_properties();
}


api_witness_schedule_object db_api::get_witness_schedule()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_witness_schedule();
   });
}

api_witness_schedule_object db_api_impl::get_witness_schedule()
{
   return api_witness_schedule_object( _db.get_witness_schedule_object() );
}


api_hardfork_property_object db_api::get_hardfork_properties()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_hardfork_properties();
   });
}

api_hardfork_property_object db_api_impl::get_hardfork_properties()
{
   return _db.get_hardfork_property_object();
}


vector< api_reward_fund_object > db_api::get_reward_funds()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_reward_funds();
   });
}

vector< api_reward_fund_object > db_api_impl::get_reward_funds()
{
   vector< api_reward_fund_object > result;

   const auto& rf_idx = _db.get_index< reward_fund_index, by_id >();
   auto itr = rf_idx.begin();

   while( itr != rf_idx.end() )
   {
      result.push_back( *itr );
      ++itr;
   }

   return result;
}


price db_api::get_current_price_feed()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_current_price_feed();
   });
}

price db_api_impl::get_current_price_feed()
{
   return _db.get_feed_history().current_median_history;;
}


api_feed_history_object db_api::get_feed_history()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_feed_history();
   });
}

api_feed_history_object db_api_impl::get_feed_history()
{
   return _db.get_feed_history();
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< api_witness_object > db_api::list_witnesses( const list_witnesses_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_witnesses( p );
   });
}

vector< api_witness_object > db_api_impl::list_witnesses( const list_witnesses_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_witness_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_name ):
      {
         iterate_results< chain::witness_index, chain::by_name >(
            p.start.as< protocol::account_name_type >(),
            result,
            p.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      case( by_vote_name ):
      {
         auto key = p.start.as< std::pair< share_type, account_name_type > >();
         iterate_results< chain::witness_index, chain::by_vote_name >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      case( by_schedule_time ):
      {
         auto key = p.start.as< std::pair< fc::uint128, account_name_type > >();
         auto wit_id = _db.get< chain::witness_object, chain::by_name >( key.second ).id;
         iterate_results< chain::witness_index, chain::by_schedule_time >(
            boost::make_tuple( key.first, wit_id ),
            result,
            p.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_witness_object > db_api::find_witnesses( const find_witnesses_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_witnesses( p );
   });
}

vector< api_witness_object > db_api_impl::find_witnesses( const find_witnesses_params& p )
{
   FC_ASSERT( p.owners.size() <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_witness_object > result;

   for( auto& o : p.owners )
   {
      auto witness = _db.find< chain::witness_object, chain::by_name >( o );

      if( witness != nullptr )
         result.push_back( api_witness_object( *witness ) );
   }

   return result;
}


vector< api_witness_vote_object > db_api::list_witness_votes( const list_witness_votes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_witness_votes( p );
   });
}

vector< api_witness_vote_object > db_api_impl::list_witness_votes( const list_witness_votes_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_witness_vote_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_account_witness ):
      {
         auto key = p.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::witness_vote_index, chain::by_account_witness >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); } );
         break;
      }
      case( by_witness_account ):
      {
         auto key = p.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::witness_vote_index, chain::by_witness_account >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< account_name_type > db_api::get_active_witnesses()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_active_witnesses();
   });
}

vector< account_name_type > db_api_impl::get_active_witnesses()
{
   const auto& wso = _db.get_witness_schedule_object();
   size_t n = wso.current_shuffled_witnesses.size();
   vector< account_name_type > result;
   result.reserve( n );
   for( size_t i=0; i<n; i++ )
      result.push_back( wso.current_shuffled_witnesses[i] );
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

/* Accounts */

vector< api_account_object > db_api::list_accounts( const list_accounts_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_accounts( p );
   });
}

vector< api_account_object > db_api_impl::list_accounts( const list_accounts_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_account_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_name ):
      {
         iterate_results< chain::account_index, chain::by_name >(
            p.start.as< protocol::account_name_type >(),
            result,
            p.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_proxy ):
      {
         auto key = p.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::account_index, chain::by_proxy >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_next_vesting_withdrawal ):
      {
         auto key = p.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::account_index, chain::by_next_vesting_withdrawal >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_account_object > db_api::find_accounts( const find_accounts_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_accounts( p );
   });
}

vector< api_account_object > db_api_impl::find_accounts( const find_accounts_params& p )
{
   vector< api_account_object > result;
   FC_ASSERT( p.accounts.size() <= DB_API_SINGLE_QUERY_LIMIT );

   for( auto& a : p.accounts )
   {
      auto acct = _db.find< chain::account_object, chain::by_name >( a );
      if( acct != nullptr )
         result.push_back( api_account_object( *acct, _db ) );
   }

   return result;
}


/* Owner Auth Histories */

vector< api_owner_authority_history_object > db_api::list_owner_histories( const list_owner_histories_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_owner_histories( p );
   });
}

vector< api_owner_authority_history_object > db_api_impl::list_owner_histories( const list_owner_histories_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_owner_authority_history_object > result;
   result.reserve( p.limit );

   auto key = p.start.as< std::pair< account_name_type, fc::time_point_sec > >();
   iterate_results< chain::owner_authority_history_index, chain::by_account >(
      boost::make_tuple( key.first, key.second ),
      result,
      p.limit,
      [&]( const owner_authority_history_object& o ){ return api_owner_authority_history_object( o ); } );

   return result;
}


vector< api_owner_authority_history_object > db_api::find_owner_histories( const find_owner_histories_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_owner_histories( p );
   });
}

vector< api_owner_authority_history_object > db_api_impl::find_owner_histories( const find_owner_histories_params& p )
{
   vector< api_owner_authority_history_object > result;

   const auto& hist_idx = _db.get_index< chain::owner_authority_history_index, chain::by_account >();
   auto itr = hist_idx.lower_bound( p.owner );

   while( itr != hist_idx.end() && itr->account == p.owner && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( api_owner_authority_history_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Account Recovery Requests */

vector< api_account_recovery_request_object > db_api::list_account_recovery_requests( const list_account_recovery_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_account_recovery_requests( p );
   });
}

vector< api_account_recovery_request_object > db_api_impl::list_account_recovery_requests( const list_account_recovery_requests_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_account_recovery_request_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_account ):
      {
         iterate_results< chain::account_recovery_request_index, chain::by_account >(
            p.start.as< account_name_type >(),
            result,
            p.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); } );
         break;
      }
      case( by_expiration ):
      {
         auto key = p.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::account_recovery_request_index, chain::by_expiration >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_account_recovery_request_object > db_api::find_account_recovery_requests( const find_account_recovery_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_account_recovery_requests( p );
   });
}

vector< api_account_recovery_request_object > db_api_impl::find_account_recovery_requests( const find_account_recovery_requests_params& p )
{
   vector< api_account_recovery_request_object > result;
   FC_ASSERT( p.accounts.size() <= DB_API_SINGLE_QUERY_LIMIT );

   for( auto& a : p.accounts )
   {
      auto request = _db.find< chain::account_recovery_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.push_back( api_account_recovery_request_object( *request ) );
   }

   return result;
}


/* Change Recovery Account Requests */

vector< api_change_recovery_account_request_object > db_api::list_change_recovery_account_requests( const list_change_recovery_account_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_change_recovery_account_requests( p );
   });
}

vector< api_change_recovery_account_request_object > db_api_impl::list_change_recovery_account_requests( const list_change_recovery_account_requests_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_change_recovery_account_request_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_account ):
      {
         iterate_results< chain::change_recovery_account_request_index, chain::by_account >(
            p.start.as< account_name_type >(),
            result,
            p.limit,
            &db_api_impl::on_push_default< change_recovery_account_request_object > );
         break;
      }
      case( by_effective_date ):
      {
         auto key = p.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::change_recovery_account_request_index, chain::by_effective_date >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< change_recovery_account_request_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_change_recovery_account_request_object > db_api::find_change_recovery_account_requests( const find_change_recovery_account_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_change_recovery_account_requests( p );
   });
}

vector< api_change_recovery_account_request_object > db_api_impl::find_change_recovery_account_requests( const find_change_recovery_account_requests_params& p )
{
   vector< api_change_recovery_account_request_object > result;
   FC_ASSERT( p.accounts.size() <= DB_API_SINGLE_QUERY_LIMIT );

   for( auto& a : p.accounts )
   {
      auto request = _db.find< chain::change_recovery_account_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.push_back( *request );
   }

   return result;
}


/* Escrows */

vector< api_escrow_object > db_api::list_escrows( const list_escrows_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_escrows( p );
   });
}

vector< api_escrow_object > db_api_impl::list_escrows( const list_escrows_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_escrow_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_from_id ):
      {
         auto key = p.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::escrow_index, chain::by_from_id >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< escrow_object > );
         break;
      }
      case( by_ratification_deadline ):
      {
         auto key = p.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_ratification_deadline start requires 3 values. (bool, time_point_sec, escrow_id_type)" );
         iterate_results< chain::escrow_index, chain::by_ratification_deadline >(
            boost::make_tuple( key[0].as< bool >(), key[1].as< fc::time_point_sec >(), key[2].as< escrow_id_type >() ),
            result,
            p.limit,
            &db_api_impl::on_push_default< escrow_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_escrow_object > db_api::find_escrows( const find_escrows_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_escrows( p );
   });
}

vector< api_escrow_object > db_api_impl::find_escrows( const find_escrows_params& p )
{
   vector< api_escrow_object > result;

   const auto& escrow_idx = _db.get_index< chain::escrow_index, chain::by_from_id >();
   auto itr = escrow_idx.lower_bound( p.from );

   while( itr != escrow_idx.end() && itr->from == p.from && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Withdraw Vesting Routes */

vector< api_withdraw_vesting_route_object > db_api::list_withdraw_vesting_routes( const list_withdraw_vesting_routes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_withdraw_vesting_routes( p );
   });
}

vector< api_withdraw_vesting_route_object > db_api_impl::list_withdraw_vesting_routes( const list_withdraw_vesting_routes_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_withdraw_vesting_route_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_withdraw_route ):
      {
         auto key = p.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::withdraw_vesting_route_index, chain::by_withdraw_route >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< withdraw_vesting_route_object > );
         break;
      }
      case( by_destination ):
      {
         auto key = p.start.as< std::pair< account_name_type, withdraw_vesting_route_id_type > >();
         iterate_results< chain::withdraw_vesting_route_index, chain::by_destination >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< withdraw_vesting_route_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_withdraw_vesting_route_object > db_api::find_withdraw_vesting_routes( const find_withdraw_vesting_routes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_withdraw_vesting_routes( p );
   });
}

vector< api_withdraw_vesting_route_object > db_api_impl::find_withdraw_vesting_routes( const find_withdraw_vesting_routes_params& p )
{
   vector< api_withdraw_vesting_route_object > result;

   switch( p.order )
   {
      case( by_withdraw_route ):
      {
         const auto& route_idx = _db.get_index< chain::withdraw_vesting_route_index, chain::by_withdraw_route >();
         auto itr = route_idx.lower_bound( p.account );

         while( itr != route_idx.end() && itr->from_account == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
         {
            result.push_back( *itr );
            ++itr;
         }

         break;
      }
      case( by_destination ):
      {
         const auto& route_idx = _db.get_index< chain::withdraw_vesting_route_index, chain::by_destination >();
         auto itr = route_idx.lower_bound( p.account );

         while( itr != route_idx.end() && itr->to_account == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
         {
            result.push_back( *itr );
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

vector< api_savings_withdraw_object > db_api::list_savings_withdrawals( const list_savings_withdrawals_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_savings_withdrawals( p );
   });
}

vector< api_savings_withdraw_object > db_api_impl::list_savings_withdrawals( const list_savings_withdrawals_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_savings_withdraw_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_from_id ):
      {
         auto key = p.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::savings_withdraw_index, chain::by_from_rid >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const savings_withdraw_object& w ){ return api_savings_withdraw_object( w ); } );
         break;
      }
      case( by_complete_from_id ):
      {
         auto key = p.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_complete_from_id start requires 3 values. (time_point_sec, account_name_type, uint32_t)" );
         iterate_results< chain::savings_withdraw_index, chain::by_complete_from_rid >(
            boost::make_tuple( key[0].as< fc::time_point_sec >(), key[1].as< account_name_type >(), key[2].as< uint32_t >() ),
            result,
            p.limit,
            [&]( const savings_withdraw_object& w ){ return api_savings_withdraw_object( w ); } );
         break;
      }
      case( by_to_complete ):
      {
         auto key = p.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_to_complete start requires 3 values. (account_name_type, time_point_sec, savings_withdraw_id_type" );
         iterate_results< chain::savings_withdraw_index, chain::by_to_complete >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< fc::time_point_sec >(), key[2].as< savings_withdraw_id_type >() ),
            result,
            p.limit,
            [&]( const savings_withdraw_object& w ){ return api_savings_withdraw_object( w ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_savings_withdraw_object > db_api::find_savings_withdrawals( const find_savings_withdrawals_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_savings_withdrawals( p );
   });
}

vector< api_savings_withdraw_object > db_api_impl::find_savings_withdrawals( const find_savings_withdrawals_params& p )
{
   vector< api_savings_withdraw_object > result;
   const auto& withdraw_idx = _db.get_index< chain::savings_withdraw_index, chain::by_from_rid >();
   auto itr = withdraw_idx.lower_bound( p.account );

   while( itr != withdraw_idx.end() && itr->from == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( api_savings_withdraw_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Vesting Delegations */

vector< api_vesting_delegation_object > db_api::list_vesting_delegations( const list_vesting_delegations_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_vesting_delegations( p );
   });
}

vector< api_vesting_delegation_object > db_api_impl::list_vesting_delegations( const list_vesting_delegations_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_vesting_delegation_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_delegation ):
      {
         auto key = p.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::vesting_delegation_index, chain::by_delegation >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_vesting_delegation_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

vector< api_vesting_delegation_object > db_api::find_vesting_delegations( const find_vesting_delegations_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_vesting_delegations( p );
   });
}

vector< api_vesting_delegation_object > db_api_impl::find_vesting_delegations( const find_vesting_delegations_params& p )
{
   vector< api_vesting_delegation_object > result;
   const auto& delegation_idx = _db.get_index< chain::vesting_delegation_index, chain::by_delegation >();
   auto itr = delegation_idx.lower_bound( p.account );

   while( itr != delegation_idx.end() && itr->delegator == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( api_vesting_delegation_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Vesting Delegation Expirations */

vector< api_vesting_delegation_expiration_object > db_api::list_vesting_delegation_expirations( const list_vesting_delegation_expirations_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_vesting_delegation_expirations( p );
   });
}

vector< api_vesting_delegation_expiration_object > db_api_impl::list_vesting_delegation_expirations( const list_vesting_delegation_expirations_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_vesting_delegation_expiration_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_expiration ):
      {
         auto key = p.start.as< std::pair< time_point_sec, vesting_delegation_expiration_id_type > >();
         iterate_results< chain::vesting_delegation_expiration_index, chain::by_expiration >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_vesting_delegation_expiration_object > );
         break;
      }
      case( by_account_expiration ):
      {
         auto key = p.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_account_expiration start requires 3 values. (account_name_type, time_point_sec, vesting_delegation_expiration_id_type" );
         iterate_results< chain::vesting_delegation_expiration_index, chain::by_account_expiration >(
            boost::make_tuple( key[0].as< account_name_type >(), key[1].as< time_point_sec >(), key[2].as< vesting_delegation_expiration_id_type >() ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_vesting_delegation_expiration_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_vesting_delegation_expiration_object > db_api::find_vesting_delegation_expirations( const find_vesting_delegation_expirations_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_vesting_delegation_expirations( p );
   });
}

vector< api_vesting_delegation_expiration_object > db_api_impl::find_vesting_delegation_expirations( const find_vesting_delegation_expirations_params& p )
{
   vector< api_vesting_delegation_expiration_object > result;
   const auto& del_exp_idx = _db.get_index< chain::vesting_delegation_expiration_index, chain::by_account_expiration >();
   auto itr = del_exp_idx.lower_bound( p.account );

   while( itr != del_exp_idx.end() && itr->delegator == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( *itr );
      ++itr;
   }

   return result;
}


/* SBD Conversion Requests */

vector< api_convert_request_object > db_api::list_sbd_conversion_requests( const list_sbd_conversion_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_sbd_conversion_requests( p );
   });
}

vector< api_convert_request_object > db_api_impl::list_sbd_conversion_requests( const list_sbd_conversion_requests_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_convert_request_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_conversion_date ):
      {
         auto key = p.start.as< std::pair< time_point_sec, convert_request_id_type > >();
         iterate_results< chain::convert_request_index, chain::by_conversion_date >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_convert_request_object > );
         break;
      }
      case( by_account ):
      {
         auto key = p.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::convert_request_index, chain::by_owner >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_convert_request_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_convert_request_object > db_api::find_sbd_conversion_requests( const find_sbd_conversion_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_sbd_conversion_requests( p );
   });
}

vector< api_convert_request_object > db_api_impl::find_sbd_conversion_requests( const find_sbd_conversion_requests_params& p )
{
   vector< api_convert_request_object > result;
   const auto& convert_idx = _db.get_index< chain::convert_request_index, chain::by_owner >();
   auto itr = convert_idx.lower_bound( p.account );

   while( itr != convert_idx.end() && itr->owner == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Decline Voting Rights Requests */

vector< api_decline_voting_rights_request_object > db_api::list_decline_voting_rights_requests( const list_decline_voting_rights_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_decline_voting_rights_requests( p );
   });
}

vector< api_decline_voting_rights_request_object > db_api_impl::list_decline_voting_rights_requests( const list_decline_voting_rights_requests_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_decline_voting_rights_request_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_account ):
      {
         iterate_results< chain::decline_voting_rights_request_index, chain::by_account >(
            p.start.as< account_name_type >(),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_decline_voting_rights_request_object > );
         break;
      }
      case( by_effective_date ):
      {
         auto key = p.start.as< std::pair< time_point_sec, account_name_type > >();
         iterate_results< chain::decline_voting_rights_request_index, chain::by_effective_date >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_decline_voting_rights_request_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_decline_voting_rights_request_object > db_api::find_decline_voting_rights_requests( const find_decline_voting_rights_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_decline_voting_rights_requests( p );
   });
}

vector< api_decline_voting_rights_request_object > db_api_impl::find_decline_voting_rights_requests( const find_decline_voting_rights_requests_params& p )
{
   FC_ASSERT( p.accounts.size() <= DB_API_SINGLE_QUERY_LIMIT );
   vector< api_decline_voting_rights_request_object > result;

   for( auto& a : p.accounts )
   {
      auto request = _db.find< chain::decline_voting_rights_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.push_back( *request );
   }

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Comments                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

/* Comments */

vector< api_comment_object > db_api::list_comments( const list_comments_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_comments( p );
   });
}

vector< api_comment_object > db_api_impl::list_comments( const list_comments_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_comment_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_cashout_time ):
      {
         auto key = p.start.as< vector< fc::variant > >();
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
            result,
            p.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_permlink ):
      {
         auto key = p.start.as< std::pair< account_name_type, string > >();
         iterate_results< chain::comment_index, chain::by_permlink >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_root ):
      {
         auto key = p.start.as< vector< fc::variant > >();
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
            result,
            p.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_parent ):
      {
         auto key = p.start.as< vector< fc::variant > >();
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
            result,
            p.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
#ifndef IS_LOW_MEM
      case( by_last_update ):
      {
         auto key = p.start.as< vector< fc::variant > >();
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
            result,
            p.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
      case( by_author_last_update ):
      {
         auto key = p.start.as< vector< fc::variant > >();
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
            result,
            p.limit,
            [&]( const comment_object& c ){ return api_comment_object( c, _db ); } );
         break;
      }
#endif
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_comment_object > db_api::find_comments( const find_comments_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_comments( p );
   });
}

vector< api_comment_object > db_api_impl::find_comments( const find_comments_params& p )
{
   FC_ASSERT( p.comments.size() <= DB_API_SINGLE_QUERY_LIMIT );
   vector< api_comment_object > result;
   result.reserve( p.comments.size() );

   for( auto& key: p.comments )
   {
      auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( key.first, key.second ) );

      if( comment != nullptr )
         result.push_back( api_comment_object( *comment, _db ) );
   }

   return result;
}


/* Votes */

vector< api_comment_vote_object > db_api::list_votes( const list_votes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_votes( p );
   });
}

vector< api_comment_vote_object > db_api_impl::list_votes( const list_votes_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_comment_vote_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_comment_voter ):
      {
         auto key = p.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_comment_voter start requires 3 values. (account_name_type, string, account_name_type)" );

         auto author = key[0].as< account_name_type >();
         auto permlink = key[1].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         auto voter = key[2].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< chain::account_object, chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         iterate_results< chain::comment_vote_index, chain::by_comment_voter >(
            boost::make_tuple( comment_id, voter_id ),
            result,
            p.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      case( by_voter_comment ):
      {
         auto key = p.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_comment_voter start requires 3 values. (account_name_type, account_name_type, string)" );

         auto voter = key[1].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< chain::account_object, chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         auto author = key[1].as< account_name_type >();
         auto permlink = key[2].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< chain::comment_vote_index, chain::by_voter_comment >(
            boost::make_tuple( voter_id, comment_id ),
            result,
            p.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      case( by_voter_last_update ):
      {
         auto key = p.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_comment_voter start requires 4 values. (account_name_type, time_point_sec, account_name_type, string)" );

         auto voter = key[1].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< chain::account_object, chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         auto author = key[2].as< account_name_type >();
         auto permlink = key[3].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         iterate_results< chain::comment_vote_index, chain::by_voter_last_update >(
            boost::make_tuple( voter_id, key[1].as< fc::time_point_sec >(), comment_id ),
            result,
            p.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      case( by_comment_weight_voter ):
      {
         auto key = p.start.as< vector< fc::variant > >();
         FC_ASSERT( key.size() == 4, "by_comment_voter start requires 4 values. (account_name_type, string, uint64_t, account_name_type)" );

         auto author = key[0].as< account_name_type >();
         auto permlink = key[1].as< string >();
         comment_id_type comment_id;

         if( author != account_name_type() || permlink.size() )
         {
            auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( author, permlink ) );
            FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}.", ("a", author)("p", permlink) );
            comment_id = comment->id;
         }

         auto voter = key[3].as< account_name_type >();
         account_id_type voter_id;

         if( voter != account_name_type() )
         {
            auto account = _db.find< chain::account_object, chain::by_name >( voter );
            FC_ASSERT( account != nullptr, "Could not find voter ${v}.", ("v", voter ) );
            voter_id = account->id;
         }

         iterate_results< chain::comment_vote_index, chain::by_comment_weight_voter >(
            boost::make_tuple( comment_id, key[2].as< uint64_t >(), voter_id ),
            result,
            p.limit,
            [&]( const comment_vote_object& cv ){ return api_comment_vote_object( cv, _db ); } );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_comment_vote_object > db_api::find_votes( const find_votes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_votes( p );
   });
}

vector< api_comment_vote_object > db_api_impl::find_votes( const find_votes_params& p )
{
   vector< api_comment_vote_object > result;

   auto comment = _db.find< chain::comment_object, chain::by_permlink >( boost::make_tuple( p.author, p.permlink ) );
   FC_ASSERT( comment != nullptr, "Could not find comment ${a}/${p}", ("a", p.author)("p", p.permlink ) );

   const auto& vote_idx = _db.get_index< chain::comment_vote_index, chain::by_comment_voter >();
   auto itr = vote_idx.lower_bound( comment->id );

   while( itr != vote_idx.end() && itr->comment == comment->id && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( api_comment_vote_object( *itr, _db ) );
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

vector< api_limit_order_object > db_api::list_limit_orders( const list_limit_orders_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_limit_orders( p );
   });
}

vector< api_limit_order_object > db_api_impl::list_limit_orders( const list_limit_orders_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );

   vector< api_limit_order_object > result;
   result.reserve( p.limit );

   switch( p.order )
   {
      case( by_price ):
      {
         auto key = p.start.as< std::pair< price, limit_order_id_type > >();
         iterate_results< chain::limit_order_index, chain::by_price >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_limit_order_object > );
         break;
      }
      case( by_account ):
      {
         auto key = p.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::limit_order_index, chain::by_account >(
            boost::make_tuple( key.first, key.second ),
            result,
            p.limit,
            &db_api_impl::on_push_default< api_limit_order_object > );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}


vector< api_limit_order_object > db_api::find_limit_orders( const find_limit_orders_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->find_limit_orders( p );
   });
}

vector< api_limit_order_object > db_api_impl::find_limit_orders( const find_limit_orders_params& p )
{
   vector< api_limit_order_object > result;
   const auto& order_idx = _db.get_index< chain::limit_order_index, chain::by_account >();
   auto itr = order_idx.lower_bound( p.account );

   while( itr != order_idx.end() && itr->seller == p.account && result.size() <= DB_API_SINGLE_QUERY_LIMIT )
   {
      result.push_back( *itr );
      ++itr;
   }

   return result;
}


/* Order Book */

order_book db_api::get_order_book( const get_order_book_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_order_book( p );
   });
}

order_book db_api_impl::get_order_book( const get_order_book_params& p )
{
   FC_ASSERT( p.limit <= DB_API_SINGLE_QUERY_LIMIT );
   order_book result;

   auto max_sell = price::max( SBD_SYMBOL, STEEM_SYMBOL );
   auto max_buy = price::max( STEEM_SYMBOL, SBD_SYMBOL );

   const auto& limit_price_idx = _db.get_index< chain::limit_order_index >().indices().get< chain::by_price >();
   auto sell_itr = limit_price_idx.lower_bound( max_sell );
   auto buy_itr  = limit_price_idx.lower_bound( max_buy );
   auto end = limit_price_idx.end();

   while( sell_itr != end && sell_itr->sell_price.base.symbol == SBD_SYMBOL && result.bids.size() < p.limit )
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
   while( buy_itr != end && buy_itr->sell_price.base.symbol == STEEM_SYMBOL && result.asks.size() < p.limit )
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

std::string db_api::get_transaction_hex( const get_transaction_hex_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_transaction_hex( p );
   });
}

std::string db_api_impl::get_transaction_hex( const get_transaction_hex_params& p )
{
   return fc::to_hex( fc::raw::pack( p.trx ) );
}


annotated_signed_transaction db_api::get_transaction( const get_transaction_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_transaction( p );
   });
}

annotated_signed_transaction db_api_impl::get_transaction( const get_transaction_params& p )
{
   return _db.with_read_lock( [&](){
      const auto& idx = _db.get_index< chain::operation_index, chain::by_transaction_id >();
      auto itr = idx.lower_bound( p.id );
      if( itr != idx.end() && itr->trx_id == p.id )
      {
         auto blk = _db.fetch_block_by_number( itr->block );
         FC_ASSERT( blk.valid() );
         FC_ASSERT( blk->transactions.size() > itr->trx_in_block );
         annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
         result.block_num       = itr->block;
         result.transaction_num = itr->trx_in_block;
         return result;
      }
      FC_ASSERT( false, "Unknown Transaction ${t}", ("t", p.id) );
   });
}


set< public_key_type > db_api::get_required_signatures( const get_required_signatures_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_required_signatures( p );
   });
}

set< public_key_type > db_api_impl::get_required_signatures( const get_required_signatures_params& p )
{
   auto result = p.trx.get_required_signatures( STEEMIT_CHAIN_ID,
                                                p.available_keys,
                                                [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).active  ); },
                                                [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner   ); },
                                                [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting ); },
                                                STEEMIT_MAX_SIG_CHECK_DEPTH );

   return result;
}


set< public_key_type > db_api::get_potential_signatures( const get_potential_signatures_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_potential_signatures( p );
   });
}

set< public_key_type > db_api_impl::get_potential_signatures( const get_potential_signatures_params& p )
{
   set< public_key_type > result;
   p.trx.get_required_signatures(
      STEEMIT_CHAIN_ID,
      flat_set< public_key_type >(),
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).active;
         for( const auto& k : auth.get_keys() )
            result.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner;
         for( const auto& k : auth.get_keys() )
            result.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting;
         for( const auto& k : auth.get_keys() )
            result.insert( k );
         return authority( auth );
      },
      STEEMIT_MAX_SIG_CHECK_DEPTH
   );

   return result;
}


bool db_api::verify_authority( const verify_authority_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_authority( p );
   });
}

bool db_api_impl::verify_authority( const verify_authority_params& p )
{
   p.trx.verify_authority( STEEMIT_CHAIN_ID,
                           [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).active  ); },
                           [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).owner   ); },
                           [&]( string account_name ){ return authority( _db.get< chain::account_authority_object, chain::by_account >( account_name ).posting ); },
                           STEEMIT_MAX_SIG_CHECK_DEPTH );
   return true;
}


bool db_api::verify_account_authority( const verify_account_authority_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_account_authority( p );
   });
}

// TODO: This is broken. By the look of is, it has been since BitShares. verify_authority always
// returns false because the TX is not signed.
bool db_api_impl::verify_account_authority( const verify_account_authority_params& p )
{
   auto account = _db.find< chain::account_object, chain::by_name >( p.account );
   FC_ASSERT( account != nullptr, "no such account" );

   /// reuse trx.verify_authority by creating a dummy transfer
   verify_authority_params vap;
   transfer_operation op;
   op.from = account->name;
   vap.trx.operations.emplace_back( op );

   return verify_authority( vap );
}

} } // steemit::db_api
