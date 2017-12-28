#include <golos/plugins/tolstoy_api/tolstoy_api.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
    namespace plugins {
        namespace tolstoy_api {

            using fc::variant;

            struct tolstoy_api::tolstoy_api_impl final {
            public:
                tolstoy_api_impl() : rpc_(appbase::app().get_plugin<json_rpc::plugin>()) {
                }

                json_rpc::plugin &rpc() {
                    return rpc_;
                }

            private:
                json_rpc::plugin &rpc_;
            };

            tolstoy_api::tolstoy_api() : pimpl(new tolstoy_api_impl) {
                JSON_RPC_REGISTER_API(plugin_name)
            }

            tolstoy_api::~tolstoy_api() {

            }

            DEFINE_API(tolstoy_api, get_block_header) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_block) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_ops_in_block) {
                return pimpl->rpc().call(args);
            }




            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Globals                                                          //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_config) {
                return pimpl->rpc().call(args);

            }

            DEFINE_API(tolstoy_api, get_dynamic_global_properties) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_chain_properties) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_feed_history) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_current_median_history_price) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_witness_schedule) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_hardfork_version) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_next_scheduled_hardfork) {
                return pimpl->rpc().call(args);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Accounts                                                         //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_accounts) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, lookup_account_names) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, lookup_accounts) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_account_count) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_owner_history) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_recovery_request) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_escrow) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_withdraw_routes) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_account_bandwidth) {
                return pimpl->rpc().call(args);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Witnesses                                                        //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_witnesses) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_witness_by_account) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_witnesses_by_vote) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, lookup_witness_accounts) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_witness_count) {
                return pimpl->rpc().call(args);
            }



            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Balances                                                         //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_account_balances) {
                return pimpl->rpc().call(args);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Assets                                                           //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_assets) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_assets_by_issuer) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_assets_dynamic_data) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_bitassets_data) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, list_assets) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, lookup_asset_symbols) {
                return pimpl->rpc().call(args);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Authority / validation                                           //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_transaction_hex) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_required_signatures) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_potential_signatures) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, verify_authority) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, verify_account_authority) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_conversion_requests) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_content) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_active_votes) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_account_votes) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_content_replies) {
                return pimpl->rpc().call(args);
            }

            /**
             *  This method can be used to fetch replies to an account.
             *
             *  The first call should be (account_to_retrieve replies, "", limit)
             *  Subsequent calls should be (last_author, last_permlink, limit)
             */
            DEFINE_API(tolstoy_api, get_replies_by_last_update) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_account_history) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_tags_used_by_author) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_trending_tags) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_trending) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_post_discussions_by_payout) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_comment_discussions_by_payout) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_promoted) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_created) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_active) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_cashout) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_payout) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_votes) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_children) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_discussions_by_hot) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_feed) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_discussions_by_blog) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_comments) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_trending_categories) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_best_categories) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_active_categories) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_recent_categories) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_miner_queue) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_active_witnesses) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_discussions_by_author_before_date) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_savings_withdraw_from) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_savings_withdraw_to) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_vesting_delegations) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_expiring_vesting_delegations) {
                return pimpl->rpc().call(args);
            }

            DEFINE_API(tolstoy_api, get_transaction) {
                return pimpl->rpc().call(args);
            }


            DEFINE_API(tolstoy_api, get_reward_fund) {
                return pimpl->rpc().call(args);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Proposed transactions                                            //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(tolstoy_api, get_proposed_transactions) {
                return pimpl->rpc().call(args);
            }
        }
    }
}
