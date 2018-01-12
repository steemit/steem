#ifndef GOLOS_TOLSTOY_API_PLUGIN_HPP
#define GOLOS_TOLSTOY_API_PLUGIN_HPP

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <fc/variant.hpp>


namespace golos {
    namespace plugins {
        namespace tolstoy_api {
            using fc::variant;
            using std::vector;
            using json_rpc::void_type;
            using plugins::json_rpc::msg_pack;

            ///               API,                                    args,                return
            DEFINE_API_ARGS(get_trending_tags, msg_pack, variant)
            DEFINE_API_ARGS(get_active_witnesses, msg_pack, variant)
            DEFINE_API_ARGS(get_block_header, msg_pack, variant)
            DEFINE_API_ARGS(get_block, msg_pack, variant)
            DEFINE_API_ARGS(get_ops_in_block, msg_pack, variant)
            DEFINE_API_ARGS(get_config, msg_pack, variant)
            DEFINE_API_ARGS(get_dynamic_global_properties, msg_pack, variant)
            DEFINE_API_ARGS(get_chain_properties, msg_pack, variant)
            DEFINE_API_ARGS(get_current_median_history_price, msg_pack, variant)
            DEFINE_API_ARGS(get_feed_history, msg_pack, variant)
            DEFINE_API_ARGS(get_witness_schedule, msg_pack, variant)
            DEFINE_API_ARGS(get_hardfork_version, msg_pack, variant)
            DEFINE_API_ARGS(get_next_scheduled_hardfork, msg_pack, variant)
            DEFINE_API_ARGS(get_reward_fund, msg_pack, variant)
            DEFINE_API_ARGS(get_key_references, msg_pack, variant)
            DEFINE_API_ARGS(get_accounts, msg_pack, variant)
            DEFINE_API_ARGS(lookup_account_names, msg_pack, variant)
            DEFINE_API_ARGS(lookup_accounts, msg_pack, variant)
            DEFINE_API_ARGS(get_account_count, msg_pack, variant)
            DEFINE_API_ARGS(get_owner_history, msg_pack, variant)
            DEFINE_API_ARGS(get_recovery_request, msg_pack, variant)
            DEFINE_API_ARGS(get_escrow, msg_pack, variant)
            DEFINE_API_ARGS(get_withdraw_routes, msg_pack, variant)
            DEFINE_API_ARGS(get_account_bandwidth, msg_pack, variant)
            DEFINE_API_ARGS(get_savings_withdraw_from, msg_pack, variant)
            DEFINE_API_ARGS(get_savings_withdraw_to, msg_pack, variant)
            DEFINE_API_ARGS(get_vesting_delegations, msg_pack, variant)
            DEFINE_API_ARGS(get_expiring_vesting_delegations, msg_pack, variant)
            DEFINE_API_ARGS(get_witnesses, msg_pack, variant)
            DEFINE_API_ARGS(get_conversion_requests, msg_pack, variant)
            DEFINE_API_ARGS(get_witness_by_account, msg_pack, variant)
            DEFINE_API_ARGS(get_witnesses_by_vote, msg_pack, variant)
            DEFINE_API_ARGS(lookup_witness_accounts, msg_pack, variant)
            DEFINE_API_ARGS(get_open_orders, msg_pack, variant)
            DEFINE_API_ARGS(get_witness_count, msg_pack, variant)
            DEFINE_API_ARGS(get_transaction_hex, msg_pack, variant)
            DEFINE_API_ARGS(get_transaction, msg_pack, variant)
            DEFINE_API_ARGS(get_required_signatures, msg_pack, variant)
            DEFINE_API_ARGS(get_potential_signatures, msg_pack, variant)
            DEFINE_API_ARGS(verify_authority, msg_pack, variant)
            DEFINE_API_ARGS(verify_account_authority, msg_pack, variant)
            DEFINE_API_ARGS(get_active_votes, msg_pack, variant)
            DEFINE_API_ARGS(get_account_votes, msg_pack, variant)
            DEFINE_API_ARGS(get_content, msg_pack, variant)
            DEFINE_API_ARGS(get_content_replies, msg_pack, variant)
            DEFINE_API_ARGS(get_tags_used_by_author, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_payout, msg_pack, variant)
            DEFINE_API_ARGS(get_post_discussions_by_payout, msg_pack, variant)
            DEFINE_API_ARGS(get_comment_discussions_by_payout, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_trending, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_created, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_active, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_cashout, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_votes, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_children, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_hot, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_feed, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_blog, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_comments, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_promoted, msg_pack, variant)
            DEFINE_API_ARGS(get_replies_by_last_update, msg_pack, variant)
            DEFINE_API_ARGS(get_discussions_by_author_before_date, msg_pack, variant)
            DEFINE_API_ARGS(get_account_history, msg_pack, variant)
            ///asset
            DEFINE_API_ARGS(get_assets_dynamic_data, msg_pack, variant)
            DEFINE_API_ARGS(get_assets_by_issuer, msg_pack, variant)
            ///categories
            DEFINE_API_ARGS(get_trending_categories, msg_pack, variant)

            ///
            DEFINE_API_ARGS(get_account_balances, msg_pack, variant)
            DEFINE_API_ARGS(get_active_categories, msg_pack, variant)
            DEFINE_API_ARGS(get_recent_categories, msg_pack, variant)
            DEFINE_API_ARGS(get_proposed_transactions, msg_pack, variant)
            DEFINE_API_ARGS(list_assets, msg_pack, variant)
            DEFINE_API_ARGS(get_bitassets_data, msg_pack, variant)
            DEFINE_API_ARGS(get_miner_queue, msg_pack, variant)
            DEFINE_API_ARGS(get_assets, msg_pack, variant)
            DEFINE_API_ARGS(get_best_categories, msg_pack, variant)
            DEFINE_API_ARGS(lookup_asset_symbols, msg_pack, variant)

            class tolstoy_api final {
            public:
                constexpr static const char *plugin_name = "tolstoy_api";

                tolstoy_api();

                ~tolstoy_api();

                DECLARE_API((get_trending_tags)

                                    /**
                                     *  This API is a short-cut for returning all of the state required for a particular URL
                                     *  with a single query.
                                     */

                                    (get_trending_categories)

                                    (get_best_categories)

                                    (get_active_categories)

                                    (get_recent_categories)

                                    (get_active_witnesses)

                                    (get_miner_queue)

                                    /////////////////////////////
                                    // Blocks and transactions //
                                    /////////////////////////////

                                    /**
                                     * @brief Retrieve a block header
                                     * @param block_num Height of the block whose header should be returned
                                     * @return header of the referenced block, or null if no matching block was found
                                     */

                                    (get_block_header)

                                    /**
                                     * @brief Retrieve a full, signed block
                                     * @param block_num Height of the block to be returned
                                     * @return the referenced block, or null if no matching block was found
                                     */
                                    (get_block)

                                    /**
                                     *  @brief Get sequence of operations included/generated within a particular block
                                     *  @param block_num Height of the block whose generated virtual operations should be returned
                                     *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
                                     *  @return sequence of operations included/generated within the block
                                     */
                                    (get_ops_in_block)

                                    /////////////
                                    // Globals //
                                    /////////////

                                    /**
                                     * @brief Retrieve compile-time constants
                                     */
                                    (get_config)

                                    /**
                                     * @brief Retrieve the current @ref dynamic_global_property_object
                                     */
                                    (get_dynamic_global_properties)

                                    (get_chain_properties)

                                    (get_current_median_history_price)

                                    (get_feed_history)

                                    (get_witness_schedule)

                                    (get_hardfork_version)

                                    (get_next_scheduled_hardfork)

                                    (get_reward_fund)

                                    //////////////
                                    // Accounts //
                                    //////////////

                                    (get_accounts)

                                    /**
                                     * @brief Get a list of accounts by name
                                     * @param account_names Names of the accounts to retrieve
                                     * @return The accounts holding the provided names
                                     *
                                     * This function has semantics identical to @ref get_objects
                                     */
                                    (lookup_account_names)

                                    /**
                                     * @brief Get names and IDs for registered accounts
                                     * @param lower_bound_name Lower bound of the first name to return
                                     * @param limit Maximum number of results to return -- must not exceed 1000
                                     * @return Map of account names to corresponding IDs
                                     */
                                    (lookup_accounts)

                                    //////////////
                                    // Balances //
                                    //////////////

                                    /**
                                     * @brief Get an account's balances in various assets
                                     * @param name of the account to get balances for
                                     * @param assets names of the assets to get balances of; if empty, get all assets account has a balance in
                                     * @return Balances of the account
                                     */
                                    (get_account_balances)

                                    /**
                                     * @brief Get the total number of accounts registered with the blockchain
                                     */
                                    (get_account_count)

                                    (get_owner_history)

                                    (get_recovery_request)

                                    (get_escrow)

                                    (get_withdraw_routes)

                                    (get_account_bandwidth)

                                    (get_savings_withdraw_from)

                                    (get_savings_withdraw_to)

                                    (get_vesting_delegations)

                                    (get_expiring_vesting_delegations)

                                    ///////////////
                                    // Witnesses //
                                    ///////////////

                                    /**
                                     * @brief Get a list of witnesses by ID
                                     * @param witness_ids IDs of the witnesses to retrieve
                                     * @return The witnesses corresponding to the provided IDs
                                     *
                                     * This function has semantics identical to @ref get_objects
                                     */
                                    (get_witnesses)

                                    (get_conversion_requests)

                                    /**
                                     * @brief Get the witness owned by a given account
                                     * @param account The name of the account whose witness should be retrieved
                                     * @return The witness object, or null if the account does not have a witness
                                     */
                                    (get_witness_by_account)

                                    /**
                                     *  This method is used to fetch witnesses with pagination.
                                     *
                                     *  @return an array of `count` witnesses sorted by total votes after witness `from` with at most `limit' results.
                                     */
                                    (get_witnesses_by_vote)

                                    /**
                                     * @brief Get names and IDs for registered witnesses
                                     * @param lower_bound_name Lower bound of the first name to return
                                     * @param limit Maximum number of results to return -- must not exceed 1000
                                     * @return Map of witness names to corresponding IDs
                                     */
                                    (lookup_witness_accounts)

                                    /**
                                     * @brief Get the total number of witnesses registered with the blockchain
                                     */
                                    (get_witness_count)

                                    ////////////
                                    // Assets //
                                    ////////////

                                    /**
                                     * @brief Get a list of assets by ID
                                     * @param asset_symbols IDs of the assets to retrieve
                                     * @return The assets corresponding to the provided IDs
                                     *
                                     * This function has semantics identical to @ref get_objects
                                     */
                                    (get_assets)

                                    (get_assets_by_issuer)

                                    (get_assets_dynamic_data)

                                    (get_bitassets_data)

                                    /**
                                     * @brief Get assets alphabetically by symbol name
                                     * @param lower_bound_symbol Lower bound of symbol names to retrieve
                                     * @param limit Maximum number of assets to fetch (must not exceed 100)
                                     * @return The assets found
                                     */
                                    (list_assets)

                                    /**
                                     * @brief Get a list of assets by symbol
                                     * @param asset_symbols Symbols or stringified IDs of the assets to retrieve
                                     * @return The assets corresponding to the provided symbols or IDs
                                     *
                                     * This function has semantics identical to @ref get_objects
                                     */
                                    (lookup_asset_symbols)

                                    ////////////////////////////
                                    // Authority / Validation //
                                    ////////////////////////////

                                    /// @brief Get a hexdump of the serialized binary form of a transaction
                                    (get_transaction_hex)

                                    (get_transaction)

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

                                    /*
                                     * @return true if the signers have enough authority to authorize an account
                                     */
                                    (verify_account_authority)

                                    /**
                                     *  if permlink is "" then it will return all votes for author
                                     */
                                    (get_active_votes)

                                    (get_account_votes)


                                    (get_content)

                                    (get_content_replies)

                                    /**
                                     * Used to retrieve top 1000 tags list used by an author sorted by most frequently used
                                     * @param author select tags of this author
                                     * @return vector of top 1000 tags used by an author sorted by most frequently used
                                     **/
                                    (get_tags_used_by_author)

                                    /**
                                     * Used to retrieve the list of first payout discussions sorted by rshares^2 amount
                                     * @param query @ref discussion_query
                                     * @return vector of first payout mode discussions sorted by rshares^2 amount
                                     **/
                                    (get_discussions_by_trending)

                                    /**
                                     * Used to retrieve the list of discussions sorted by created time
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by created time
                                     **/
                                    (get_discussions_by_created)

                                    /**
                                     * Used to retrieve the list of discussions sorted by last activity time
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by last activity time
                                     **/
                                    (get_discussions_by_active)

                                    /**
                                     * Used to retrieve the list of discussions sorted by cashout time
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by last cashout time
                                     **/
                                    (get_discussions_by_cashout)

                                    /**
                                     * Used to retrieve the list of discussions sorted by net rshares amount
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by net rshares amount
                                     **/
                                    (get_discussions_by_payout)

                                    (get_post_discussions_by_payout)

                                    (get_comment_discussions_by_payout)

                                    /**
                                     * Used to retrieve the list of discussions sorted by direct votes amount
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by direct votes amount
                                     **/
                                    (get_discussions_by_votes)

                                    /**
                                     * Used to retrieve the list of discussions sorted by children posts amount
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by children posts amount
                                     **/
                                    (get_discussions_by_children)

                                    /**
                                     * Used to retrieve the list of discussions sorted by hot amount
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by hot amount
                                     **/
                                    (get_discussions_by_hot)

                                    /**
                                     * Used to retrieve the list of discussions from the feed of a specific author
                                     * @param query @ref discussion_query
                                     * @attention @ref discussion_query#select_authors must be set and must contain the @ref discussion_query#start_author param if the last one is not null
                                     * @return vector of discussions from the feed of authors in @ref discussion_query#select_authors
                                     **/
                                    (get_discussions_by_feed)

                                    /**
                                     * Used to retrieve the list of discussions from the blog of a specific author
                                     * @param query @ref discussion_query
                                     * @attention @ref discussion_query#select_authors must be set and must contain the @ref discussion_query#start_author param if the last one is not null
                                     * @return vector of discussions from the blog of authors in @ref discussion_query#select_authors
                                     **/
                                    (get_discussions_by_blog)

                                    (get_discussions_by_comments)

                                    /**
                                     * Used to retrieve the list of discussions sorted by promoted balance amount
                                     * @param query @ref discussion_query
                                     * @return vector of discussions sorted by promoted balance amount
                                     **/
                                    (get_discussions_by_promoted)


                                    /**
                                     *  Return the active discussions with the highest cumulative pending payouts without respect to category, total
                                     *  pending payout means the pending payout of all children as well.
                                     */
                                    (get_replies_by_last_update)


                                    /**
                                     *  This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink with up to limit being returned.
                                     *
                                     *  If start_permlink is empty then only before_date will be considered. If both are specified the eariler to the two metrics will be used. This
                                     *  should allow easy pagination.
                                     */
                                    (get_discussions_by_author_before_date)

                                    /**
                                     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
                                     *  returns operations in the range [from-limit, from]
                                     *
                                     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
                                     *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
                                     */
                                    (get_account_history)



                                    ///////////////////////////
                                    // Proposed transactions //
                                    ///////////////////////////

                                    /**
                                     *  @return the set of proposed transactions relevant to the specified account id.
                                     */
                                    (get_proposed_transactions)

                )

            private:
                struct tolstoy_api_impl;
                std::unique_ptr<tolstoy_api_impl> pimpl;
            };
        }
    }
}

#endif //GOLOS_TOLSTOY_API_PLUGIN_HPP
