#include <golos/plugins/mongo_db/mongo_db_state.hpp>
#include <golos/plugins/follow/follow_objects.hpp>
#include <golos/plugins/follow/plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/chain/comment_object.hpp>
#include <golos/chain/account_object.hpp>

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/value_context.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <appbase/plugin.hpp>


namespace golos {
namespace plugins {
namespace mongo_db {

    using bsoncxx::builder::stream::array;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using namespace golos::plugins::follow;

    state_writer::state_writer(db_map& bmi_to_add, const signed_block& block) :
        db_(appbase::app().get_plugin<golos::plugins::chain::plugin>().db()),
        state_block(block),
        all_docs(bmi_to_add) {
    }

    named_document state_writer::create_document(const std::string& name,
            const std::string& key, const std::string& keyval) {
        named_document doc;
        doc.collection_name = name;
        doc.key = key;
        doc.keyval = keyval;
        doc.is_removal = false;
        return doc;
    }

    named_document state_writer::create_removal_document(const std::string& name,
            const std::string& key, const std::string& keyval) {
        named_document doc;
        doc.collection_name = name;
        doc.key = key;
        doc.keyval = keyval;
        doc.is_removal = true;
        return doc;
    }

    void state_writer::format_comment(const std::string& auth, const std::string& perm) {
        try {
            auto& comment = db_.get_comment(auth, perm);
            auto oid = std::string(auth).append("/").append(perm);
            auto oid_hash = fc::sha1::hash(oid).str().substr(0, 24);

            auto doc = create_document("comment_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "removed", false);

            format_value(body, "author", auth);
            format_value(body, "permlink", perm);
            format_value(body, "abs_rshares", comment.abs_rshares);
            format_value(body, "active", comment.active);

            format_value(body, "allow_curation_rewards", comment.allow_curation_rewards);
            format_value(body, "allow_replies", comment.allow_replies);
            format_value(body, "allow_votes", comment.allow_votes);
            format_value(body, "author_rewards", comment.author_rewards);
            format_value(body, "beneficiary_payout", comment.beneficiary_payout_value);
            format_value(body, "cashout_time", comment.cashout_time);
            format_value(body, "children", comment.children);
            format_value(body, "children_abs_rshares", comment.children_abs_rshares);
            format_value(body, "children_rshares2", comment.children_rshares2);
            format_value(body, "created", comment.created);
            format_value(body, "curator_payout", comment.curator_payout_value);
            format_value(body, "depth", comment.depth);
            format_value(body, "last_payout", comment.last_payout);
            format_value(body, "last_update", comment.last_update);
            format_value(body, "max_accepted_payout", comment.max_accepted_payout);
            format_value(body, "max_cashout_time", comment.max_cashout_time);
            format_value(body, "net_rshares", comment.net_rshares);
            format_value(body, "net_votes", comment.net_votes);
            format_value(body, "parent_author", comment.parent_author);
            format_value(body, "parent_permlink", comment.parent_permlink);
            format_value(body, "percent_steem_dollars", comment.percent_steem_dollars);
            format_value(body, "reward_weight", comment.reward_weight);
            format_value(body, "total_payout", comment.total_payout_value);
            format_value(body, "total_vote_weight", comment.total_vote_weight);
            format_value(body, "vote_rshares", comment.vote_rshares);

            if (!comment.beneficiaries.empty()) {
                array ben_array;
                for (auto& b: comment.beneficiaries) {
                    document tmp;
                    format_value(tmp, "account", b.account);
                    format_value(tmp, "weight", b.weight);
                    ben_array << tmp;
                }
                body << "beneficiaries" << ben_array;
            }

            std::string comment_mode;
            switch (comment.mode) {
                case first_payout:
                    comment_mode = "first_payout";
                    break;
                case second_payout:
                    comment_mode = "second_payout";
                    break;
                case archived:
                    comment_mode = "archived";
                    break;
            }

            format_value(body, "mode", comment_mode);

            auto& content = db_.get_comment_content(comment_id_type(comment.id));

            format_value(body, "title", content.title);
            format_value(body, "body", content.body);
            format_value(body, "json_metadata", content.json_metadata);

            std::string category, root_oid;
            if (comment.parent_author == STEEMIT_ROOT_POST_PARENT) {
                category = to_string(comment.parent_permlink);
                root_oid = oid;
            } else {
                auto& root_comment = db_.get<comment_object, by_id>(comment.root_comment);
                category = to_string(root_comment.parent_permlink);
                root_oid = std::string(root_comment.author).append("/").append(root_comment.permlink.c_str());
            }
            format_value(body, "category", category);
            format_oid(body, "root_comment", root_oid);
            document root_comment_index;
            root_comment_index << "root_comment" << 1;
            doc.indexes_to_create.push_back(std::move(root_comment_index));

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
//        catch (fc::exception& ex) {
//            ilog("MongoDB operations fc::exception during formatting comment. ${e}", ("e", ex.what()));
//        }
        catch (...) {
            // ilog("Unknown exception during formatting comment.");
        }
    }

    auto state_writer::operator()(const vote_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
        
        try {
            auto& vote_idx = db_.get_index<comment_vote_index>().indices().get<by_comment_voter>();
            auto& comment = db_.get_comment(op.author, op.permlink);
            auto& voter = db_.get_account(op.voter);
            auto itr = vote_idx.find(std::make_tuple(comment.id, voter.id));
            if (vote_idx.end() != itr) {
                auto comment_oid = std::string(op.author).append("/").append(op.permlink);
                auto oid = comment_oid + "/" + op.voter;
                auto oid_hash = fc::sha1::hash(oid).str().substr(0, 24);

                auto doc = create_document("comment_vote_object", "_id", oid_hash);
                document comment_index;
                comment_index << "comment" << 1;
                doc.indexes_to_create.push_back(std::move(comment_index));
                auto &body = doc.doc;

                body << "$set" << open_document;

                format_oid(body, oid);
                format_oid(body, "comment", comment_oid);

                format_value(body, "author", op.author);
                format_value(body, "permlink", op.permlink);
                format_value(body, "voter", op.voter);

                format_value(body, "weight", itr->weight);
                format_value(body, "rshares", itr->rshares);
                format_value(body, "vote_percent", itr->vote_percent);
                format_value(body, "last_update", itr->last_update);
                format_value(body, "num_changes", itr->num_changes);

                body << close_document;

                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        }
//        catch (fc::exception& ex) {
//            ilog("MongoDB operations fc::exception during formatting vote. ${e}", ("e", ex.what()));
//        }
        catch (...) {
            // ilog("Unknown exception during formatting vote.");
        }
    }

    auto state_writer::operator()(const comment_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
    }

    auto state_writer::operator()(const comment_options_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
    }
    
    auto state_writer::operator()(const delete_comment_operation& op) -> result_type {

	std::string author = op.author;

        auto comment_oid = std::string(op.author).append("/").append(op.permlink);
        auto comment_oid_hash = fc::sha1::hash(comment_oid).str().substr(0, 24);

        // Will be updated with the following fields. If no one - created with these fields.
	auto comment = create_document("comment_object", "_id", comment_oid_hash);

        auto& body = comment.doc;

        body << "$set" << open_document;

        format_oid(body, comment_oid);

        format_value(body, "removed", true);

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);

        body << close_document;

        bmi_insert_or_replace(all_docs, std::move(comment));

        // Will be updated with removed = true. If no one - nothing to do.
	auto comment_vote = create_removal_document("comment_vote_object", "comment", comment_oid_hash);
        
        bmi_insert_or_replace(all_docs, std::move(comment_vote));
    }

    auto state_writer::operator()(const transfer_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const transfer_to_vesting_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const withdraw_vesting_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const limit_order_create_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const limit_order_cancel_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const feed_publish_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const convert_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const account_create_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const account_update_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const witness_update_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const account_witness_vote_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const account_witness_proxy_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const pow_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const custom_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const report_over_production_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const custom_json_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const set_withdraw_vesting_route_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const limit_order_create2_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const challenge_authority_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const prove_authority_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const request_account_recovery_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const recover_account_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const change_recovery_account_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const escrow_transfer_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const escrow_dispute_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const escrow_release_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const pow2_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const escrow_approve_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const transfer_to_savings_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const transfer_from_savings_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const cancel_transfer_from_savings_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const custom_binary_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const decline_voting_rights_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const reset_account_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const set_reset_account_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const delegate_vesting_shares_operation& op) -> result_type {
        
    }
    auto state_writer::operator()(const account_create_with_delegation_operation& op) -> result_type {
        
    }
    auto state_writer::operator()(const account_metadata_operation& op) -> result_type {
        
    }
    auto state_writer::operator()(const proposal_create_operation& op) -> result_type {
        
    }
    auto state_writer::operator()(const proposal_update_operation& op) -> result_type {
        
    }
    auto state_writer::operator()(const proposal_delete_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const fill_convert_request_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const liquidity_reward_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const interest_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const fill_vesting_withdraw_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const fill_order_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const shutdown_witness_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const fill_transfer_from_savings_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const hardfork_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const comment_payout_update_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
    }

    auto state_writer::operator()(const author_reward_operation& op) -> result_type {
        try {
            auto comment_oid = std::string(op.author).append("/").append(op.permlink);
            auto comment_oid_hash = fc::sha1::hash(comment_oid).str().substr(0, 24);

            auto doc = create_document("author_reward", "_id", comment_oid_hash);
            auto &body = doc.doc;

            body << "$set" << open_document;

            format_value(body, "removed", false);
            format_oid(body, comment_oid);
            format_oid(body, "comment", comment_oid);
            format_value(body, "author", op.author);
            format_value(body, "permlink", op.permlink);
            format_value(body, "timestamp", state_block.timestamp);
            format_value(body, "sbd_payout", op.sbd_payout);
            format_value(body, "steem_payout", op.steem_payout);
            format_value(body, "vesting_payout", op.vesting_payout);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));

        } catch (...) {
            //
        }
    }

    auto state_writer::operator()(const curation_reward_operation& op) -> result_type {
        try {
            auto comment_oid = std::string(op.comment_author).append("/").append(op.comment_permlink);
            auto vote_oid = comment_oid + "/" + op.curator;
            auto vote_oid_hash = fc::sha1::hash(vote_oid).str().substr(0, 24);

            auto doc = create_document("curation_reward", "_id", vote_oid_hash);
            document comment_index;
            comment_index << "comment" << 1;
            doc.indexes_to_create.push_back(std::move(comment_index));
            auto &body = doc.doc;

            body << "$set" << open_document;

            format_value(body, "removed", false);
            format_oid(body, vote_oid);
            format_oid(body, "comment", comment_oid);
            format_oid(body, "vote", vote_oid);
            format_value(body, "author", op.comment_author);
            format_value(body, "permlink", op.comment_permlink);
            format_value(body, "timestamp", state_block.timestamp);
            format_value(body, "reward", op.reward);
            format_value(body, "curator", op.curator);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        } catch (...) {
            //
        }
    }

    auto state_writer::operator()(const comment_reward_operation& op) -> result_type {
        try {
            auto comment_oid = std::string(op.author).append("/").append(op.permlink);
            auto comment_oid_hash = fc::sha1::hash(comment_oid).str().substr(0, 24);

            auto doc = create_document("comment_reward", "_id", comment_oid_hash);
            auto &body = doc.doc;

            body << "$set" << open_document;

            format_value(body, "removed", false);
            format_oid(body, comment_oid);
            format_oid(body, "comment", comment_oid);
            format_value(body, "author", op.author);
            format_value(body, "permlink", op.permlink);
            format_value(body, "timestamp", state_block.timestamp);
            format_value(body, "payout", op.payout);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        } catch (...) {
            //
        }
    }

    auto state_writer::operator()(const comment_benefactor_reward_operation& op) -> result_type {
        try {
            auto comment_oid = std::string(op.author).append("/").append(op.permlink);
            auto benefactor_oid = comment_oid + "/" + op.benefactor;
            auto benefactor_oid_hash = fc::sha1::hash(benefactor_oid).str().substr(0, 24);

            auto doc = create_document("benefactor_reward", "_id", benefactor_oid_hash);       
            document comment_index;
            comment_index << "comment" << 1;
            doc.indexes_to_create.push_back(std::move(comment_index));
            auto &body = doc.doc;

            body << "$set" << open_document;

            format_value(body, "removed", false);
            format_oid(body, benefactor_oid);
            format_oid(body, "comment", comment_oid);
            format_value(body, "author", op.author);
            format_value(body, "permlink", op.permlink);
            format_value(body, "timestamp", state_block.timestamp);
            format_value(body, "reward", op.reward);
            format_value(body, "benefactor", op.benefactor);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        } catch (...) {
            //
        }
    }

    auto state_writer::operator()(const return_vesting_delegation_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const chain_properties_update_operation& op) -> result_type {
        
    }

}}}