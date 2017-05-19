#pragma once

#include <steemit/app/application.hpp>

#include <steemit/db_api/db_api_params.hpp>
#include <steemit/db_api/db_api_objects.hpp>

#define DB_API_SINGLE_QUERY_LIMIT 1000

namespace steemit { namespace db_api {

class db_api_impl;

class db_api
{
   public:
      db_api( const steemit::app::api_context& ctx );
      ~db_api();


      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional< block_header > get_block_header( const get_block_header_params& p );

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional< api_signed_block_object > get_block( const get_block_params& p );

      /**
       *  @brief Get sequence of operations included/generated within a particular block
       *  @param block_num Height of the block whose generated virtual operations should be returned
       *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
       *  @return sequence of operations included/generated within the block
       */
      vector< api_operation_object > get_ops_in_block( const get_ops_in_block_params& p );

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config();

       /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      api_dynamic_global_property_object get_dynamic_global_properties();

      api_witness_schedule_object get_witness_schedule();

      api_hardfork_property_object get_hardfork_properties();

      vector< api_reward_fund_object > get_reward_funds();

      price get_current_price_feed();

      api_feed_history_object get_feed_history();


      ///////////////
      // Witnesses //
      ///////////////

      vector< api_witness_object > list_witnesses( const list_witnesses_params& p );
      vector< api_witness_object > find_witnesses( const find_witnesses_params& p );

      vector< api_witness_vote_object > list_witness_votes( const list_witness_votes_params& p );

      vector< account_name_type > get_active_witnesses();


      //////////////
      // Accounts //
      //////////////

      /**
       * @brief List accounts ordered by specified key
       *
       */
      vector< api_account_object > list_accounts( const list_accounts_params& p );

      /**
       * @brief Find accounts by primary key (account name)
       */
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


      //////////////
      // Comments //
      //////////////

      vector< api_comment_object > list_comments( const list_comments_params& p );
      vector< api_comment_object > find_comments( const find_comments_params& p );

      vector< api_comment_vote_object > list_votes( const list_votes_params& p );
      vector< api_comment_vote_object > find_votes( const find_votes_params& p );


      ////////////
      // Market //
      ////////////

      vector< api_limit_order_object > list_limit_orders( const list_limit_orders_params& p );
      vector< api_limit_order_object > find_limit_orders( const find_limit_orders_params& p );

      order_book get_order_book( const get_order_book_params& p );


      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string                   get_transaction_hex( const get_transaction_hex_params& p );
      annotated_signed_transaction  get_transaction( const get_transaction_params& p );

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const get_required_signatures_params& p );

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const get_potential_signatures_params& p );

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const verify_authority_params& p );

      /**
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const verify_account_authority_params& p );


      ////////////////////////////
      // Handlers - not exposed //
      ////////////////////////////
      void on_api_startup();

   private:
      std::shared_ptr< db_api_impl > my;
};

} } //steemit::db_api

FC_API( steemit::db_api::db_api,
   // Blocks and transactions
   (get_block_header)
   (get_block)
   (get_ops_in_block)

   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_witness_schedule)
   (get_hardfork_properties)
   (get_reward_funds)
   (get_current_price_feed)
   (get_feed_history)

   // Witnesses
   (list_witnesses)
   (find_witnesses)
   (list_witness_votes)
   (get_active_witnesses)

   // Accounts
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

   // Comments
   (list_comments)
   (find_comments)
   (list_votes)
   (find_votes)

   // Market
   (list_limit_orders)
   (find_limit_orders)
   (get_order_book)

   // Authority / Validation
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)
)
