#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <fc/optional.hpp>
#include <fc/variant_object.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/database_api/state.hpp>
#include <golos/plugins/database_api/api_objects/feed_history_api_object.hpp>
#include <golos/plugins/database_api/api_objects/owner_authority_history_api_object.hpp>
#include <golos/plugins/database_api/api_objects/account_recovery_request_api_object.hpp>
#include <golos/plugins/database_api/api_objects/savings_withdraw_api_object.hpp>
#include <golos/plugins/chain/plugin.hpp>

#include "forward.hpp"

namespace golos { namespace plugins { namespace database_api {
            using namespace golos::chain;
            using namespace golos::protocol;
            using fc::variant;
            using std::vector;
            using plugins::json_rpc::void_type;
            using plugins::json_rpc::msg_pack;
            using plugins::json_rpc::msg_pack_transfer;

            struct database_index_info {
                std::string name;
                std::size_t record_count;
            };

            struct database_info {
                std::size_t total_size;
                std::size_t free_size;
                std::size_t reserved_size;
                std::size_t used_size;

                std::vector<database_index_info> index_list;
            };

            struct scheduled_hardfork {
                hardfork_version hf_version;
                fc::time_point_sec live_time;
            };

            struct withdraw_route {
                std::string from_account;
                std::string to_account;
                uint16_t percent;
                bool auto_vest;
            };

            enum withdraw_route_type {
                incoming, outgoing, all
            };


            struct tag_count_object {
                string tag;
                uint32_t count;
            };

            struct get_tags_used_by_author {
                vector<tag_count_object> tags;
            };

            struct signed_block_api_object : public signed_block {
                signed_block_api_object(const signed_block &block) : signed_block(block) {
                    block_id = id();
                    signing_key = signee();
                    transaction_ids.reserve(transactions.size());
                    for (const signed_transaction &tx : transactions) {
                        transaction_ids.push_back(tx.id());
                    }
                }

                signed_block_api_object() {
                }

                block_id_type block_id;
                public_key_type signing_key;
                vector<transaction_id_type> transaction_ids;
            };


            using chain_properties_17 = chain_properties;
            using price_17 = price;

            using block_applied_callback = std::function<void(const variant &block_header)>;

            ///               API,                                    args,                return
            DEFINE_API_ARGS(get_active_witnesses,             msg_pack, std::vector<account_name_type>)
            DEFINE_API_ARGS(get_block_header,                 msg_pack, optional<block_header>)
            DEFINE_API_ARGS(get_block,                        msg_pack, optional<signed_block>)
            DEFINE_API_ARGS(set_block_applied_callback,       msg_pack, void_type)
            DEFINE_API_ARGS(get_config,                       msg_pack, variant_object)
            DEFINE_API_ARGS(get_dynamic_global_properties,    msg_pack, dynamic_global_property_api_object)
            DEFINE_API_ARGS(get_chain_properties,             msg_pack, chain_properties_17)
            DEFINE_API_ARGS(get_current_median_history_price, msg_pack, price_17)
            DEFINE_API_ARGS(get_feed_history,                 msg_pack, feed_history_api_object)
            DEFINE_API_ARGS(get_witness_schedule,             msg_pack, witness_schedule_api_object)
            DEFINE_API_ARGS(get_hardfork_version,             msg_pack, hardfork_version)
            DEFINE_API_ARGS(get_next_scheduled_hardfork,      msg_pack, scheduled_hardfork)
            DEFINE_API_ARGS(get_accounts,                     msg_pack, std::vector<extended_account>)
            DEFINE_API_ARGS(lookup_account_names,             msg_pack, std::vector<optional<account_api_object> >)
            DEFINE_API_ARGS(lookup_accounts,                  msg_pack, std::set<std::string>)
            DEFINE_API_ARGS(get_account_count,                msg_pack, uint64_t)
            DEFINE_API_ARGS(get_owner_history,                msg_pack, std::vector<owner_authority_history_api_object>)
            DEFINE_API_ARGS(get_recovery_request,             msg_pack, optional<account_recovery_request_api_object>)
            DEFINE_API_ARGS(get_escrow,                       msg_pack, optional<escrow_api_object>)
            DEFINE_API_ARGS(get_withdraw_routes,              msg_pack, std::vector<withdraw_route>)
            DEFINE_API_ARGS(get_account_bandwidth,            msg_pack, optional<account_bandwidth_api_object>)
            DEFINE_API_ARGS(get_savings_withdraw_from,        msg_pack, std::vector<savings_withdraw_api_object>)
            DEFINE_API_ARGS(get_savings_withdraw_to,          msg_pack, std::vector<savings_withdraw_api_object>)

            DEFINE_API_ARGS(get_vesting_delegations,          msg_pack, vector<vesting_delegation_api_object>)
            DEFINE_API_ARGS(get_expiring_vesting_delegations, msg_pack, vector<vesting_delegation_expiration_api_object>)

            DEFINE_API_ARGS(get_witnesses,                    msg_pack, std::vector<optional<witness_api_object> >)
            DEFINE_API_ARGS(get_conversion_requests,          msg_pack, std::vector<convert_request_api_object>)
            DEFINE_API_ARGS(get_witness_by_account,           msg_pack, optional<witness_api_object>)
            DEFINE_API_ARGS(get_witnesses_by_vote,            msg_pack, std::vector<witness_api_object>)
            DEFINE_API_ARGS(lookup_witness_accounts,          msg_pack, std::set<account_name_type>)
            DEFINE_API_ARGS(get_open_orders,                  msg_pack, std::vector<extended_limit_order>)
            DEFINE_API_ARGS(get_witness_count,                msg_pack, uint64_t)
            DEFINE_API_ARGS(get_transaction_hex,              msg_pack, std::string)
            DEFINE_API_ARGS(get_required_signatures,          msg_pack, std::set<public_key_type>)
            DEFINE_API_ARGS(get_potential_signatures,         msg_pack, std::set<public_key_type>)
            DEFINE_API_ARGS(verify_authority,                 msg_pack, bool)
            DEFINE_API_ARGS(verify_account_authority,         msg_pack, bool)
            DEFINE_API_ARGS(get_miner_queue,                  msg_pack, std::vector<account_name_type>)
            DEFINE_API_ARGS(get_database_info,                msg_pack, database_info)


            /**
             * @brief The database_api class implements the RPC API for the chain database.
             *
             * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
             * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
             * the @ref network_broadcast_api.
             */
            class plugin final : public appbase::plugin<plugin> {
            public:
                constexpr static const char *plugin_name = "database_api";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                APPBASE_PLUGIN_REQUIRES(
                        (json_rpc::plugin)
                        (chain::plugin)
                )

                void set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) override{}

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override{}

                plugin();

                ~plugin();

                ///////////////////
                // Subscriptions //
                ///////////////////

                void set_subscribe_callback(std::function<void(const variant &)> cb, bool clear_filter);

                void set_pending_transaction_callback(std::function<void(const variant &)> cb);

                /**
                 * @brief Stop receiving any notifications
                 *
                 * This unsubscribes from all subscribed markets and objects.
                 */
                void cancel_all_subscriptions();


                /**
                 * @brief Clear disconnected callbacks on applied block
                 */

                void clear_block_applied_callback();

                DECLARE_API(

                                    /**
                                     *  This API is a short-cut for returning all of the state required for a particular URL
                                     *  with a single query.
                                     */



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
                                     * @brief Set callback which is triggered on each generated block
                                     * @param callback function which should be called
                                     */
                                    (set_block_applied_callback)

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
                                    // (list_vesting_delegations)
                                    // (find_vesting_delegations)
                                    // (list_vesting_delegation_expirations)
                                    // (find_vesting_delegation_expirations)

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




                                    ////////////////////////////
                                    // Authority / Validation //
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

                                    /*
                                     * @return true if the signers have enough authority to authorize an account
                                     */
                                    (verify_account_authority)


                                    (get_database_info)

                )

            private:
                struct api_impl;
                std::shared_ptr<api_impl> my;
            };


            inline void register_database_api(){
                appbase::app().register_plugin<plugin>();
            }
} } } // golos::plugins::database_api







FC_REFLECT((golos::plugins::database_api::scheduled_hardfork), (hf_version)(live_time))
FC_REFLECT((golos::plugins::database_api::withdraw_route), (from_account)(to_account)(percent)(auto_vest))

FC_REFLECT_ENUM(golos::plugins::database_api::withdraw_route_type, (incoming)(outgoing)(all))

FC_REFLECT((golos::plugins::database_api::tag_count_object), (tag)(count))

FC_REFLECT((golos::plugins::database_api::get_tags_used_by_author), (tags))

FC_REFLECT((golos::plugins::database_api::signed_block_api_object), (block_id)(signing_key)(transaction_ids))

FC_REFLECT((golos::plugins::database_api::database_index_info), (name)(record_count))
FC_REFLECT((golos::plugins::database_api::database_info), (total_size)(free_size)(reserved_size)(used_size)(index_list))
