#pragma once

#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/plugins/database_api/database_api_args.hpp>
#include <steem/plugins/database_api/database_api_objects.hpp>

#define DATABASE_API_SINGLE_QUERY_LIMIT 1000

namespace steem { namespace plugins { namespace database_api {

class database_api_impl;

class database_api
{
   public:
      database_api();
      ~database_api();

      DECLARE_API(

         /////////////
         // Globals //
         /////////////

         /**
         * @brief Retrieve compile-time constants
         */
         (get_config)

         /**
          * @brief Return version information and chain_id of running node
          */
         (get_version)

         /**
         * @brief Retrieve the current @ref dynamic_global_property_object
         */
         (get_dynamic_global_properties)
         (get_witness_schedule)
         (get_hardfork_properties)
         (get_reward_funds)
         (get_current_price_feed)
         (get_feed_history)

         ///////////////
         // Witnesses //
         ///////////////
         (list_witnesses)
         (find_witnesses)
         (list_witness_votes)
         (get_active_witnesses)

         //////////////
         // Accounts //
         //////////////

         /**
         * @brief List accounts ordered by specified key
         *
         */
         (list_accounts)

         /**
         * @brief Find accounts by primary key (account name)
         */
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

         //////////////
         // Comments //
         //////////////
         (list_comments)
         (find_comments)
         (list_votes)
         (find_votes)

         ////////////
         // Market //
         ////////////
         (list_limit_orders)
         (find_limit_orders)
         (get_order_book)

         ////////////////////////////
         // Authority / validation //
         ////////////////////////////

         /// @brief Get a hexdump of the serialized binary form of a transaction
         (get_transaction_hex)

         /**
         *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
         *  and return the minimal subset of public keys that should add signatures to the transaction.
         */
         (get_required_signatures)

         /**
         *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
         *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
         *  to get the minimum subset.
         */
         (get_potential_signatures)

         /**
         * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
         */
         (verify_authority)

         /**
         * @return true if the signers have enough authority to authorize an account
         */
         (verify_account_authority)

         /*
          * This is a general purpose API that checks signatures against accounts for an arbitrary sha256 hash
          * using the existing authority structures in Steem
          */
         (verify_signatures)

#ifdef STEEM_ENABLE_SMT
         /**
         * @return array of Numeric Asset Identifier (NAI) available to be used for new SMT to be created.
         */
         (get_nai_pool)

         (list_smt_tokens)
         (find_smt_tokens)

         (list_smt_token_emissions)
         (find_smt_token_emissions)
#endif
      )

   private:
      std::unique_ptr< database_api_impl > my;
};

} } } //steem::plugins::database_api

