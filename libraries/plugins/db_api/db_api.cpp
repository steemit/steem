#include <steemit/db_api/db_api.hpp>

namespace steemit { namespace db_api {

class db_api_impl : public std::enable_shared_from_this< db_api_impl >
{
   public:
      db_api_impl( const steemit::app::api_context& ctx );
      ~db_api_impl();


      // Blocks and transactions
      optional< block_header > get_block_header( const get_block_header_params& p );
      optional< api_signed_block_object > get_block( const get_block_params& p );
      vector< applied_operation > get_ops_in_block( const get_ops_in_block_params& p );

      // Globals
      fc::variant_object get_config();
      api_dynamic_global_property_object get_dynamic_global_properties();
      api_witness_schedule_object get_witness_schedule();
      api_hardfork_property_object get_hardfork_properties();
      vector< api_reward_fund_object > get_reward_funds( const get_reward_funds_params& p );
      price get_current_price_feed();
      api_feed_history_object get_feed_history();


      // Witnesses
      vector< api_witness_object > list_witnesses( const list_witnesses_params& p );
      vector< api_witness_object > find_witnesses( const find_witnesses_params& p );

      vector< api_witness_vote_object > list_witness_votes( const list_witness_votes_params& p );

      vector< api_witness_object > get_active_witnesses();


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
   optional< block_header > result;
   return result;
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
   optional< api_signed_block_object > result;
   return result;
}


vector< applied_operation > db_api::get_ops_in_block( const get_ops_in_block_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_ops_in_block( p );
   });
}

vector< applied_operation > db_api_impl::get_ops_in_block( const get_ops_in_block_params& p )
{
   vector< applied_operation > result;
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
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
   fc::variant_object result;
   return result;
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
   api_dynamic_global_property_object result;
   return result;
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
   api_witness_schedule_object result;
   return result;
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
   api_hardfork_property_object result;
   return result;
}


vector< api_reward_fund_object > db_api::get_reward_funds( const get_reward_funds_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_reward_funds( p );
   });
}

vector< api_reward_fund_object > db_api_impl::get_reward_funds( const get_reward_funds_params& p )
{
   vector< api_reward_fund_object > result;
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
   price result;
   return result;
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
   api_feed_history_object result;
   return result;
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
   vector< api_witness_object > result;
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
   vector< api_witness_object > result;
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
   vector< api_witness_vote_object > result;
   return result;
}


vector< api_witness_object > db_api::get_active_witnesses()
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_active_witnesses();
   });
}

vector< api_witness_object > db_api_impl::get_active_witnesses()
{
   vector< api_witness_object > result;
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< api_account_object > db_api::list_accounts( const list_accounts_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_accounts( p );
   });
}

vector< api_account_object > db_api_impl::list_accounts( const list_accounts_params& p )
{
   vector< api_account_object > result;
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
   return result;
}


vector< api_owner_authority_history_object > db_api::list_owner_histories( const list_owner_histories_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_owner_histories( p );
   });
}

vector< api_owner_authority_history_object > db_api_impl::list_owner_histories( const list_owner_histories_params& p )
{
   vector< api_owner_authority_history_object > result;
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
   return result;
}


vector< api_account_recovery_request_object > db_api::list_account_recovery_requests( const list_account_recovery_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_account_recovery_requests( p );
   });
}

vector< api_account_recovery_request_object > db_api_impl::list_account_recovery_requests( const list_account_recovery_requests_params& p )
{
   vector< api_account_recovery_request_object > result;
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
   return result;
}


vector< api_change_recovery_account_request_object > db_api::list_change_recovery_account_requests( const list_change_recovery_account_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_change_recovery_account_requests( p );
   });
}

vector< api_change_recovery_account_request_object > db_api_impl::list_change_recovery_account_requests( const list_change_recovery_account_requests_params& p )
{
   vector< api_change_recovery_account_request_object > result;
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
   return result;
}


vector< api_escrow_object > db_api::list_escrows( const list_escrows_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_escrows( p );
   });
}

vector< api_escrow_object > db_api_impl::list_escrows( const list_escrows_params& p )
{
   vector< api_escrow_object > result;
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
   return result;
}


vector< api_withdraw_vesting_route_object > db_api::list_withdraw_vesting_routes( const list_withdraw_vesting_routes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_withdraw_vesting_routes( p );
   });
}

vector< api_withdraw_vesting_route_object > db_api_impl::list_withdraw_vesting_routes( const list_withdraw_vesting_routes_params& p )
{
   vector< api_withdraw_vesting_route_object > result;
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
   return result;
}


vector< api_savings_withdraw_object > db_api::list_savings_withdrawals( const list_savings_withdrawals_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_savings_withdrawals( p );
   });
}

vector< api_savings_withdraw_object > db_api_impl::list_savings_withdrawals( const list_savings_withdrawals_params& p )
{
   vector< api_savings_withdraw_object > result;
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
   return result;
}


vector< api_vesting_delegation_object > db_api::list_vesting_delegations( const list_vesting_delegations_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_vesting_delegations( p );
   });
}

vector< api_vesting_delegation_object > db_api_impl::list_vesting_delegations( const list_vesting_delegations_params& p )
{
   vector< api_vesting_delegation_object > result;
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
   return result;
}


vector< api_vesting_delegation_expiration_object > db_api::list_vesting_delegation_expirations( const list_vesting_delegation_expirations_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_vesting_delegation_expirations( p );
   });
}

vector< api_vesting_delegation_expiration_object > db_api_impl::list_vesting_delegation_expirations( const list_vesting_delegation_expirations_params& p )
{
   vector< api_vesting_delegation_expiration_object > result;
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
   return result;
}


vector< api_convert_request_object > db_api::list_sbd_conversion_requests( const list_sbd_conversion_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_sbd_conversion_requests( p );
   });
}

vector< api_convert_request_object > db_api_impl::list_sbd_conversion_requests( const list_sbd_conversion_requests_params& p )
{
   vector< api_convert_request_object > result;
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
   return result;
}


vector< api_decline_voting_rights_request_object > db_api::list_decline_voting_rights_requests( const list_decline_voting_rights_requests_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_decline_voting_rights_requests( p );
   });
}

vector< api_decline_voting_rights_request_object > db_api_impl::list_decline_voting_rights_requests( const list_decline_voting_rights_requests_params& p )
{
   vector< api_decline_voting_rights_request_object > result;
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
   vector< api_decline_voting_rights_request_object > result;
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Comments                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< api_comment_object > db_api::list_comments( const list_comments_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_comments( p );
   });
}

vector< api_comment_object > db_api_impl::list_comments( const list_comments_params& p )
{
   vector< api_comment_object > result;
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
   vector< api_comment_object > result;
   return result;
}

vector< api_comment_vote_object > db_api::list_votes( const list_votes_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_votes( p );
   });
}

vector< api_comment_vote_object > db_api_impl::list_votes( const list_votes_params& p )
{
   vector< api_comment_vote_object > result;
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
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Market                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< api_limit_order_object > db_api::list_limit_orders( const list_limit_orders_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->list_limit_orders( p );
   });
}

vector< api_limit_order_object > db_api_impl::list_limit_orders( const list_limit_orders_params& p )
{
   vector< api_limit_order_object > result;
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
   return result;
}


order_book db_api::get_order_book( const get_order_book_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_order_book( p );
   });
}

order_book db_api_impl::get_order_book( const get_order_book_params& p )
{
   order_book result;
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
   std::string result;
   return result;
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
   annotated_signed_transaction result;
   return result;
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
   set< public_key_type > result;
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
   bool result = false;
   return result;
}


bool db_api::verify_account_authority( const verify_account_authority_params& p )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_account_authority( p );
   });
}

bool db_api_impl::verify_account_authority( const verify_account_authority_params& p )
{
   bool result = false;
   return result;
}

} } // steemit::db_api
