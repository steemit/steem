#pragma once

#include <steemit/plugins/json_rpc/utility.hpp>

#include <steemit/plugins/database_api/database_api_args.hpp>
#include <steemit/plugins/database_api/database_api_objects.hpp>

#define DATABASE_API_SINGLE_QUERY_LIMIT 1000

namespace steemit { namespace plugins { namespace database_api {

class database_api_impl;

class database_api
{
   public:
      database_api();
      ~database_api();


      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      DECLARE_API( get_block_header )

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      DECLARE_API( get_block )


      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve compile-time constants
       */
      DECLARE_API( get_config )

       /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      DECLARE_API( get_dynamic_global_properties )

      DECLARE_API( get_witness_schedule )

      DECLARE_API( get_hardfork_properties )

      DECLARE_API( get_reward_funds )

      DECLARE_API( get_current_price_feed )

      DECLARE_API( get_feed_history )


      ///////////////
      // Witnesses //
      ///////////////

      DECLARE_API( list_witnesses )
      DECLARE_API( find_witnesses )

      DECLARE_API( list_witness_votes )

      DECLARE_API( get_active_witnesses )


      //////////////
      // Accounts //
      //////////////

      /**
       * @brief List accounts ordered by specified key
       *
       */
      DECLARE_API( list_accounts )

      /**
       * @brief Find accounts by primary key (account name)
       */
      DECLARE_API( find_accounts )

      DECLARE_API( list_owner_histories )
      DECLARE_API( find_owner_histories )

      DECLARE_API( list_account_recovery_requests )
      DECLARE_API( find_account_recovery_requests )

      DECLARE_API( list_change_recovery_account_requests )
      DECLARE_API( find_change_recovery_account_requests )

      DECLARE_API( list_escrows )
      DECLARE_API( find_escrows )

      DECLARE_API( list_withdraw_vesting_routes )
      DECLARE_API( find_withdraw_vesting_routes )

      DECLARE_API( list_savings_withdrawals )
      DECLARE_API( find_savings_withdrawals )

      DECLARE_API( list_vesting_delegations )
      DECLARE_API( find_vesting_delegations )

      DECLARE_API( list_vesting_delegation_expirations )
      DECLARE_API( find_vesting_delegation_expirations )

      DECLARE_API( list_sbd_conversion_requests )
      DECLARE_API( find_sbd_conversion_requests )

      DECLARE_API( list_decline_voting_rights_requests )
      DECLARE_API( find_decline_voting_rights_requests )


      //////////////
      // Comments //
      //////////////

      DECLARE_API( list_comments )
      DECLARE_API( find_comments )

      DECLARE_API( list_votes )
      DECLARE_API( find_votes )


      ////////////
      // Market //
      ////////////

      DECLARE_API( list_limit_orders )
      DECLARE_API( find_limit_orders )

      DECLARE_API( get_order_book )


      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      DECLARE_API( get_transaction_hex )

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      DECLARE_API( get_required_signatures )

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      DECLARE_API( get_potential_signatures )

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      DECLARE_API( verify_authority )

      /**
       * @return true if the signers have enough authority to authorize an account
       */
      DECLARE_API( verify_account_authority )

   private:
      std::unique_ptr< database_api_impl > my;
};

} } } //steemit::plugins::database_api

