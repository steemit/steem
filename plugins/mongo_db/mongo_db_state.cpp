#include <golos/plugins/mongo_db/mongo_db_state.hpp>
#include <golos/plugins/follow/follow_objects.hpp>
#include <golos/plugins/follow/plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/chain/comment_object.hpp>
#include <golos/chain/account_object.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/proposal_object.hpp>

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/value_context.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <appbase/plugin.hpp>

#include <boost/algorithm/string.hpp>

namespace golos {
namespace plugins {
namespace mongo_db {

    using bsoncxx::builder::stream::array;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using namespace golos::plugins::follow;

    using golos::chain::by_account;
    using golos::chain::decline_voting_rights_request_index;

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

    bool state_writer::format_comment(const std::string& auth, const std::string& perm) {
        try {
            auto& comment = db_.get_comment(auth, perm);
            auto oid = std::string(auth).append("/").append(perm);
            auto oid_hash = hash_oid(oid);

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
                case not_set:
                    comment_mode = "not_set";
                    break;
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

            return true;
        }
//        catch (fc::exception& ex) {
//            ilog("MongoDB operations fc::exception during formatting comment. ${e}", ("e", ex.what()));
//        }
        catch (...) {
            // ilog("Unknown exception during formatting comment.");
            return false;
        }
    }

    void state_writer::format_account(const account_object& account) {
        try {
            auto oid = account.name;
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("account_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "name", account.name);
            format_value(body, "memo_key", std::string(account.memo_key));
            format_value(body, "proxy", account.proxy);

            format_value(body, "last_account_update", account.last_account_update);

            format_value(body, "created", account.created);
            format_value(body, "mined", account.mined);
            format_value(body, "owner_challenged", account.owner_challenged);
            format_value(body, "active_challenged", account.active_challenged);
            format_value(body, "last_owner_proved", account.last_owner_proved);
            format_value(body, "last_active_proved", account.last_active_proved);
            format_value(body, "recovery_account", account.recovery_account);
            format_value(body, "reset_account", account.reset_account);
            format_value(body, "last_account_recovery", account.last_account_recovery);
            format_value(body, "comment_count", account.comment_count);
            format_value(body, "lifetime_vote_count", account.lifetime_vote_count);
            format_value(body, "post_count", account.post_count);

            format_value(body, "can_vote", account.can_vote);
            format_value(body, "voting_power", account.voting_power);
            format_value(body, "last_vote_time", account.last_vote_time);

            format_value(body, "balance", account.balance);
            format_value(body, "savings_balance", account.savings_balance);

            format_value(body, "sbd_balance", account.sbd_balance);
            format_value(body, "sbd_seconds", account.sbd_seconds);
            format_value(body, "sbd_seconds_last_update", account.sbd_seconds_last_update);
            format_value(body, "sbd_last_interest_payment", account.sbd_last_interest_payment);

            format_value(body, "savings_sbd_balance", account.savings_sbd_balance);
            format_value(body, "savings_sbd_seconds", account.savings_sbd_seconds);
            format_value(body, "savings_sbd_seconds_last_update", account.savings_sbd_seconds_last_update);
            format_value(body, "savings_sbd_last_interest_payment", account.savings_sbd_last_interest_payment);

            format_value(body, "savings_withdraw_requests", account.savings_withdraw_requests);

            format_value(body, "curation_rewards", account.curation_rewards);
            format_value(body, "posting_rewards", account.posting_rewards);

            format_value(body, "vesting_shares", account.vesting_shares);
            format_value(body, "delegated_vesting_shares", account.delegated_vesting_shares);
            format_value(body, "received_vesting_shares", account.received_vesting_shares);

            format_value(body, "vesting_withdraw_rate", account.vesting_withdraw_rate);
            format_value(body, "next_vesting_withdrawal", account.next_vesting_withdrawal);
            format_value(body, "withdrawn", account.withdrawn);
            format_value(body, "to_withdraw", account.to_withdraw);
            format_value(body, "withdraw_routes", account.withdraw_routes);

            if (account.proxied_vsf_votes.size() != 0) {
                array ben_array;
                for (auto& b: account.proxied_vsf_votes) {
                    ben_array << b;
                }
                body << "proxied_vsf_votes" << ben_array;
            }

            format_value(body, "witnesses_voted_for", account.witnesses_voted_for);

            format_value(body, "last_post", account.last_post);

#ifndef IS_LOW_MEM
            auto& account_metadata = db_.get<account_metadata_object, by_account>(account.name);
            
            format_value(body, "json_metadata", account_metadata.json_metadata);
#endif

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) {
            // ilog("Unknown exception during formatting account.");
        }
    }

    void state_writer::format_account(const std::string& name) {
        try {
            auto& account = db_.get_account(name);
            format_account(account);
        }
        catch (...) {
            // ilog("Unknown exception during formatting account.");
        }
    }

    void state_writer::format_account_authority(const account_name_type& account_name) {
        try {
            auto& account_authority = db_.get<account_authority_object, by_account>(account_name);

            auto oid = account_authority.account;
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("account_authority_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "account", account_authority.account);

            auto owner_keys = account_authority.owner.get_keys();
            if (!owner_keys.empty()) {
                array keys_array;
                for (auto& key: owner_keys) {
                    keys_array << std::string(key);
                }
                body << "owner" << keys_array;
            }

            auto active_keys = account_authority.active.get_keys();
            if (!active_keys.empty()) {
                array keys_array;
                for (auto& key: active_keys) {
                    keys_array << std::string(key);
                }
                body << "active" << keys_array;
            }

            auto posting_keys = account_authority.posting.get_keys();
            if (!posting_keys.empty()) {
                array keys_array;
                for (auto& key: posting_keys) {
                    keys_array << std::string(key);
                }
                body << "posting" << keys_array;
            }

            format_value(body, "last_owner_update", account_authority.last_owner_update);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));

        }
        catch (...) {
            // ilog("Unknown exception during formatting account authority.");
        }
    }

    void state_writer::format_account_bandwidth(const account_name_type& account, const bandwidth_type& type) {
        try {
            auto band = db_.find<account_bandwidth_object, by_account_bandwidth_type>(
                std::make_tuple(account, type));
            if (band == nullptr) {
                return;
            }

            std::string band_type;
            switch (type) {
            case post:
                band_type = "post";
                break;
            case forum:
                band_type = "forum";
                break;
            case market:
                band_type = "market";
                break;
            }

            auto oid = std::string(band->account).append("/").append(band_type);
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("account_bandwidth_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "account", band->account);
            format_value(body, "type", band_type);
            format_value(body, "average_bandwidth", band->average_bandwidth);
            format_value(body, "lifetime_bandwidth", band->lifetime_bandwidth);
            format_value(body, "last_bandwidth_update", band->last_bandwidth_update);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));

        }
        catch (...) {
            // ilog("Unknown exception during formatting account bandwidth.");
        }
    }

    void state_writer::format_witness(const witness_object& witness) {
        try {
            auto oid = witness.owner;
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("witness_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "owner", oid);
            format_value(body, "created", witness.created);
            format_value(body, "url", witness.url); //shared_str
            format_value(body, "total_missed", witness.total_missed);
            format_value(body, "last_aslot", witness.last_aslot);
            format_value(body, "last_confirmed_block_num", witness.last_confirmed_block_num);

            format_value(body, "pow_worker", witness.pow_worker);

            format_value(body, "signing_key", std::string(witness.signing_key));

            // format_value(body, "props", witness.props); - ignored because empty
            format_value(body, "sbd_exchange_rate", witness.sbd_exchange_rate);
            format_value(body, "last_sbd_exchange_update", witness.last_sbd_exchange_update);

            format_value(body, "votes", witness.votes);
            format_value(body, "schedule", witness.schedule);

            format_value(body, "virtual_last_update", witness.virtual_last_update);
            format_value(body, "virtual_position", witness.virtual_position);
            format_value(body, "virtual_scheduled_time", witness.virtual_scheduled_time);

            format_value(body, "last_work", witness.last_work.str());

            format_value(body, "running_version", std::string(witness.running_version));

            format_value(body, "hardfork_version_vote", std::string(witness.hardfork_version_vote));
            format_value(body, "hardfork_time_vote", witness.hardfork_time_vote);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));

        }
        catch (...) {
            // ilog("Unknown exception during formatting witness.");
        }
    }

    void state_writer::format_witness(const account_name_type& owner) {
        try {
            auto& witness = db_.get_witness(owner);
            format_witness(witness);
        }
        catch (...) {
            // ilog("Unknown exception during formatting witness.");
        }
    }

    void state_writer::format_vesting_delegation_object(const vesting_delegation_object& delegation) {
        try {
            auto oid = std::string(delegation.delegator).append("/").append(std::string(delegation.delegatee));
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("vesting_delegation_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "delegator", delegation.delegator);
            format_value(body, "delegatee", delegation.delegatee);
            format_value(body, "vesting_shares", delegation.vesting_shares);
            format_value(body, "min_delegation_time", delegation.min_delegation_time);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));

        }
        catch (...) {
            // ilog("Unknown exception during formatting vesting delegation object.");
        }
    }

    void state_writer::format_vesting_delegation_object(const account_name_type& delegator, const account_name_type& delegatee) {
        try {
            auto delegation = db_.find<vesting_delegation_object, by_delegation>(std::make_tuple(delegator, delegatee));
            if (delegation == nullptr) {
                return;
            }
            format_vesting_delegation_object(*delegation);
        }
        catch (...) {
            // ilog("Unknown exception during formatting vesting delegation object.");
        }
    }

    void state_writer::format_escrow(const escrow_object &escrow) {
        try {
            auto oid = std::string(escrow.from).append("/").append(std::to_string(escrow.escrow_id));
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("escrow_object", "_id", oid_hash);

            document from_index;
            from_index << "from" << 1;
            doc.indexes_to_create.push_back(std::move(from_index));

            document to_index;
            to_index << "to" << 1;
            doc.indexes_to_create.push_back(std::move(to_index));

            document agent_index;
            agent_index << "agent" << 1;
            doc.indexes_to_create.push_back(std::move(agent_index));

            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "removed", false);
            format_value(body, "escrow_id", escrow.escrow_id);
            format_oid(body, "from", escrow.from);
            format_oid(body, "to", escrow.to);
            format_oid(body, "agent", escrow.agent);
            format_value(body, "ratification_deadline", escrow.ratification_deadline);
            format_value(body, "escrow_expiration", escrow.escrow_expiration);
            format_value(body, "sbd_balance", escrow.sbd_balance);
            format_value(body, "steem_balance", escrow.steem_balance);
            format_value(body, "pending_fee", escrow.pending_fee);
            format_value(body, "to_approved", escrow.to_approved);
            format_value(body, "agent_approved", escrow.agent_approved);
            format_value(body, "disputed", escrow.disputed);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) {
            // ilog("Unknown exception during formatting escrow.");
        }
    }

    void state_writer::format_escrow(const account_name_type &name, uint32_t escrow_id) {
        try {
            auto& escrow = db_.get_escrow(name, escrow_id);
            format_escrow(escrow);
        }
        catch (...) {
            // ilog("Unknown exception during formatting escrow.");
        }
    }

    template <typename F, typename S>
    void remove_existing(F& first, const S& second) {
        auto src = std::move(first);
        std::set_difference(
            src.begin(), src.end(),
            second.begin(), second.end(),
            std::inserter(first, first.begin()));
    }

    void state_writer::format_proposal(const proposal_object& proposal) {
        try {
            auto oid = std::string(proposal.author).append("/").append(std::string(proposal.title.c_str()));
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("proposal_object", "_id", oid_hash);
            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "removed", false);
            format_value(body, "author", proposal.author);
            format_value(body, "title", proposal.title);

            format_value(body, "memo", proposal.memo);

            if (proposal.review_period_time.valid()) {
                format_value(body, "review_period_time", *(proposal.review_period_time));
            }

            if (!proposal.proposed_operations.empty()) {
                array ops_array;
                for (auto& op: proposal.proposed_operations) {
                    ops_array << op;
                }
                body << "proposed_operations" << ops_array;
            }

            format_array_value(body, "required_active_approvals", proposal.required_active_approvals,
                [] (const account_name_type& item) -> std::string { return std::string(item); });

            format_array_value(body, "available_active_approvals", proposal.available_active_approvals,
                [] (const account_name_type& item) -> std::string { return std::string(item); });

            format_array_value(body, "required_owner_approvals", proposal.required_owner_approvals,
                [] (const account_name_type& item) -> std::string { return std::string(item); });

            format_array_value(body, "available_owner_approvals", proposal.available_owner_approvals,
                [] (const account_name_type& item) -> std::string { return std::string(item); });

            format_array_value(body, "required_posting_approvals", proposal.required_posting_approvals,
                [] (const account_name_type& item) -> std::string { return std::string(item); });

            format_array_value(body, "available_posting_approvals", proposal.available_posting_approvals,
                [] (const account_name_type& item) -> std::string { return std::string(item); });

            format_array_value(body, "available_key_approvals", proposal.available_key_approvals,
                [] (const public_key_type& item) -> std::string { return std::string(item); });

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) {
            // ilog("Unknown exception during formatting proposal object.");
        }
    }

    void state_writer::format_proposal(const account_name_type& author, const std::string& title) {
        try {
            auto& proposal = db_.get_proposal(author, title);
            format_proposal(proposal);
        }
        catch (...) {
            // ilog("Unknown exception during formatting proposal object.");
        }
    }

    void state_writer::format_required_approval(const required_approval_object& reqapp,
            const account_name_type& proposal_author, const std::string& proposal_title) {
        try {
            auto proposal_oid = std::string(proposal_author).append("/").append(proposal_title);

            auto oid = std::string(proposal_oid).append("/")
                 .append(std::string(reqapp.account));
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("required_approval_object", "_id", oid_hash);

            document proposal_index;
            proposal_index << "proposal" << 1;
            doc.indexes_to_create.push_back(std::move(proposal_index));

            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "removed", false);
            format_value(body, "account", reqapp.account);
            format_oid(body, "proposal", proposal_oid);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        } catch (...) {

        }
    }

    auto state_writer::operator()(const vote_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
        
        try {
            auto& vote_idx = db_.get_index<comment_vote_index>().indices().get<by_comment_voter>();
            auto& comment = db_.get_comment(op.author, op.permlink);
            auto& voter = db_.get_account(op.voter);
            auto comment_oid = std::string(op.author).append("/").append(op.permlink);
            auto oid = comment_oid + "/" + op.voter;
            auto oid_hash = hash_oid(oid);
            auto itr = vote_idx.find(std::make_tuple(comment.id, voter.id));
            if (vote_idx.end() != itr) {
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

                format_account(voter);

                body << close_document;

                bmi_insert_or_replace(all_docs, std::move(doc));
            } else {
                auto doc = create_removal_document("comment_vote_object", "_id", oid_hash);
                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        }
        catch (...) {
            // ilog("Unknown exception during formatting vote.");
        }
    }

    auto state_writer::operator()(const comment_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
        format_account(op.author);
        format_account_bandwidth(op.author, bandwidth_type::post);
    }

    auto state_writer::operator()(const comment_options_operation& op) -> result_type {
        format_comment(op.author, op.permlink);
    }
    
    auto state_writer::operator()(const delete_comment_operation& op) -> result_type {

	std::string author = op.author;

        auto comment_oid = std::string(op.author).append("/").append(op.permlink);
        auto comment_oid_hash = hash_oid(comment_oid);

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
        auto doc = create_document("transfer", "", "");
        auto& body = doc.doc;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "amount", op.amount);
        format_value(body, "memo", op.memo);

        std::vector<std::string> part;
        auto path = op.memo;
        boost::split(part, path, boost::is_any_of("/"));
        if (part.size() >= 2 && part[0][0] == '@') {
            auto acnt = part[0].substr(1);
            auto perm = part[1];

            if (format_comment(acnt, perm)) {
                auto comment_oid = acnt.append("/").append(perm);
                format_oid(body, "comment", comment_oid);
            }
        }

        format_account(op.from);
        format_account(op.to);

        all_docs.push_back(std::move(doc));
    }

    auto state_writer::operator()(const transfer_to_vesting_operation& op) -> result_type {
        format_account(op.from);
        format_account(op.to);
    }

    auto state_writer::operator()(const withdraw_vesting_operation& op) -> result_type {
        format_account(op.account);
    }

    auto state_writer::operator()(const limit_order_create_operation& op) -> result_type {
        format_account(op.owner);
        try {
            auto& loo = db_.get_limit_order(op.owner, op.orderid);
            auto oid = loo.seller + "/" + std::to_string(loo.orderid);
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("limit_order_object", "_id", oid_hash);

            auto &body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "created", loo.created);
            format_value(body, "expiration", loo.expiration);
            format_value(body, "seller", loo.seller);
            format_value(body, "orderid", loo.orderid);
            format_value(body, "for_sale", loo.for_sale);
            format_value(body, "sell_price", loo.sell_price);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) { 
            //...
        }
    }

    auto state_writer::operator()(const limit_order_cancel_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const feed_publish_operation& op) -> result_type {
        format_witness(op.publisher);
    }

    auto state_writer::operator()(const convert_operation& op) -> result_type {
        try {
            format_account(op.owner);

            const auto &by_owner_idx = db_.get_index<convert_request_index>().indices().get<by_owner>();
            auto itr = by_owner_idx.find(boost::make_tuple(op.owner, op.requestid));
            if (itr != by_owner_idx.end()) {
                auto oid = std::string(op.owner).append("/").append(std::to_string(op.requestid));
                auto oid_hash = hash_oid(oid);

                auto cro = create_document("convert_request_object", "_id", oid_hash);

                auto& body = cro.doc;

                body << "$set" << open_document;

                format_oid(body, oid);

                format_value(body, "owner", std::string(op.owner));
                format_value(body, "requestid", op.requestid);
                format_value(body, "amount", op.amount);
                format_value(body, "conversion_date", itr->conversion_date);

                body << close_document;

                bmi_insert_or_replace(all_docs, std::move(cro));
            }
        }
        catch (...) {
            //
        }
    }

    auto state_writer::operator()(const account_create_operation& op) -> result_type {
        format_account(op.new_account_name);
        format_account(op.creator);
        format_account_authority(op.new_account_name);
    }

    auto state_writer::operator()(const account_update_operation& op) -> result_type {
        format_account(op.account);
        format_account_authority(op.account);
    }

    auto state_writer::operator()(const account_create_with_delegation_operation& op) -> result_type {
        format_account(op.new_account_name);
        format_account(op.creator);
        format_account_authority(op.new_account_name);
        if (op.delegation.amount > 0) {
            format_vesting_delegation_object(op.creator, op.new_account_name);
        }
    }

    auto state_writer::operator()(const account_metadata_operation& op) -> result_type {
        format_account(op.account);
    }

    auto state_writer::operator()(const witness_update_operation& op) -> result_type {
        try {
            const auto &witness = db_.get_witness(op.owner);
            format_witness(witness);
        }
        catch (...) {
            // ilog("Unknown exception during formatting witness.");
        }
    }

    auto state_writer::operator()(const account_witness_vote_operation& op) -> result_type {
        try {
            const auto &voter = db_.get_account(op.account);
            const auto &witness = db_.get_witness(op.witness);

            format_account(voter);
            format_witness(witness);

            auto oid = op.witness + "/" + op.account;
            auto oid_hash = hash_oid(oid);

            const auto &by_account_witness_idx = db_.get_index<witness_vote_index>().indices().get<by_account_witness>();
            auto itr = by_account_witness_idx.find(boost::make_tuple(voter.id, witness.id));
            if (itr != by_account_witness_idx.end()) {
                auto doc = create_document("witness_vote_object", "_id", oid_hash);

                auto &body = doc.doc;

                body << "$set" << open_document;

                format_oid(body, oid);

                format_value(body, "witness", op.witness);
                format_value(body, "account", op.account);

                body << close_document;

                bmi_insert_or_replace(all_docs, std::move(doc));
            } else {
                auto doc = create_removal_document("witness_vote_object", "_id", oid_hash);
                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        }
        catch (...) {
            // ilog("Unknown exception during formatting witness vote.");
        }
    }

    auto state_writer::operator()(const account_witness_proxy_operation& op) -> result_type {
        format_account(op.account);
    }

    auto state_writer::operator()(const pow_operation& op) -> result_type {
        const auto& worker_account = op.get_worker_account();
        format_account(worker_account);
        format_account_authority(worker_account);
        format_witness(worker_account);
        const auto &dgp = db_.get_dynamic_global_properties();
        const auto &inc_witness = db_.get_account(dgp.current_witness);
        format_account(inc_witness);
        //format_global_property_object();
    }

    auto state_writer::operator()(const pow2_operation& op) -> result_type {
        const auto &dgp = db_.get_dynamic_global_properties();
        const auto &inc_witness = db_.get_account(dgp.current_witness);
        format_account(inc_witness);
        account_name_type worker_account;
        if (db_.has_hardfork(STEEMIT_HARDFORK_0_16__551)) {
            const auto &work = op.work.get<equihash_pow>();
            worker_account = work.input.worker_account;
        } else {
            const auto &work = op.work.get<pow2>();
            worker_account = work.input.worker_account;
        }
        format_account(worker_account);
        format_account_authority(worker_account);
        format_witness(worker_account);
        //format_global_property_object();
    }

    auto state_writer::operator()(const custom_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const report_over_production_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const custom_json_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const set_withdraw_vesting_route_operation& op) -> result_type {
        auto& from_account = db_.get_account(op.from_account);        
        auto& to_account = db_.get_account(op.to_account);
        format_account(from_account);
        auto oid = op.from_account + "/" + op.to_account;
        auto oid_hash = hash_oid(oid);
        try {
            auto& wvro = db_.get<withdraw_vesting_route_object, by_withdraw_route>(
                boost::make_tuple(from_account.id, to_account.id));
            auto doc = create_document("withdraw_vesting_route_object", "_id", oid_hash);

            auto &body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "from_account", std::string(from_account.name));
            format_value(body, "to_account", std::string(to_account.name));
            format_value(body, "percent", wvro.percent);
            format_value(body, "auto_vest", wvro.auto_vest);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) { 
            auto doc = create_removal_document("withdraw_vesting_route_object", "_id", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc));
        }
    }

    auto state_writer::operator()(const limit_order_create2_operation& op) -> result_type {
        format_account(op.owner);
        try {
            auto& loo = db_.get_limit_order(op.owner, op.orderid);
            auto oid = loo.seller + "/" + std::to_string(loo.orderid);
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("limit_order_object", "_id", oid_hash);

            auto &body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid);

            format_value(body, "created", loo.created);
            format_value(body, "expiration", loo.expiration);
            format_value(body, "seller", loo.seller);
            format_value(body, "orderid", loo.orderid);
            format_value(body, "for_sale", loo.for_sale);
            format_value(body, "sell_price", loo.sell_price);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) { 
            //...
        }
    }

    auto state_writer::operator()(const challenge_authority_operation& op) -> result_type {
        format_account(op.challenger);
        format_account(op.challenged);
    }

    auto state_writer::operator()(const prove_authority_operation& op) -> result_type {
        format_account(op.challenged);
    }

    auto state_writer::operator()(const request_account_recovery_operation& op) -> result_type {
        try {
            const auto &recovery_request_idx = db_.get_index<account_recovery_request_index>().indices().get<by_account>();
            auto request = recovery_request_idx.find(op.account_to_recover);
            auto oid = request->account_to_recover;
            auto oid_hash = hash_oid(oid);
            if (op.new_owner_authority.weight_threshold == 0) {
                auto doc = create_removal_document("account_recovery_request_object", "_id", oid_hash);
                bmi_insert_or_replace(all_docs, std::move(doc));
            } else if (request != recovery_request_idx.end()) {
                auto doc = create_document("account_recovery_request_object", "_id", oid_hash);

                auto& body = doc.doc;

                body << "$set" << open_document;

                format_oid(body, oid);

                format_value(body, "account_to_recover", request->account_to_recover);
                auto owner_keys = request->new_owner_authority.get_keys();
                if (!owner_keys.empty()) {
                    array keys_array;
                    for (auto& key: owner_keys) {
                        keys_array << std::string(key);
                    }
                    body << "new_owner_authority" << keys_array;
                }
                format_value(body, "expires", request->expires);

                body << close_document;

                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        }
        catch (...) {

        }
    }

    auto state_writer::operator()(const recover_account_operation& op) -> result_type {
        format_account(op.account_to_recover);
        format_account_authority(op.account_to_recover);

        auto oid = op.account_to_recover;
        auto oid_hash = hash_oid(oid);
        auto doc = create_removal_document("account_recovery_request_object", "_id", oid_hash);
        bmi_insert_or_replace(all_docs, std::move(doc));
    }

    auto state_writer::operator()(const change_recovery_account_operation& op) -> result_type {
        try {
            const auto &change_recovery_idx = db_.get_index<change_recovery_account_request_index>().indices().get<by_account>();
            auto request = change_recovery_idx.find(op.account_to_recover);
            auto oid = request->account_to_recover;
            auto oid_hash = hash_oid(oid);
            if (request != change_recovery_idx.end()) {

                auto doc = create_document("change_recovery_account_request_object", "_id", oid_hash);

                auto& body = doc.doc;

                body << "$set" << open_document;

                format_oid(body, oid);

                format_value(body, "account_to_recover", request->account_to_recover);
                format_value(body, "recovery_account", request->recovery_account);
                format_value(body, "effective_on", request->effective_on);

                body << close_document;

                bmi_insert_or_replace(all_docs, std::move(doc));
            } else {
                auto doc = create_removal_document("change_recovery_account_request_object", "_id", oid_hash);
                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        }
        catch (...) {
            //
        }
    }

    auto state_writer::operator()(const escrow_transfer_operation& op) -> result_type {
        format_account(op.from);
        format_escrow(op.from, op.escrow_id);
    }

    auto state_writer::operator()(const escrow_approve_operation& op) -> result_type {
        format_account(op.from);
        format_account(op.agent);
        if (op.approve) {
            format_escrow(op.from, op.escrow_id);
        } else {
            auto oid = std::string(op.from).append("/").append(std::to_string(op.escrow_id));
            auto oid_hash = hash_oid(oid);
            auto doc = create_removal_document("escrow_object", "_id", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc));
        }
    }

    auto state_writer::operator()(const escrow_dispute_operation& op) -> result_type {
        format_escrow(op.from, op.escrow_id);
    }

    auto state_writer::operator()(const escrow_release_operation& op) -> result_type {
        format_account(op.receiver);
        try {
            auto &escrow = db_.get_escrow(op.from, op.escrow_id);
            format_escrow(escrow);
            if (escrow.steem_balance.amount == 0 && escrow.sbd_balance.amount == 0) {
                auto oid = std::string(op.from).append("/").append(std::to_string(op.escrow_id));
                auto oid_hash = hash_oid(oid);
                auto doc = create_removal_document("escrow_object", "_id", oid_hash);
                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        } catch (...) {

        }
    }

    auto state_writer::operator()(const transfer_to_savings_operation& op) -> result_type {
        format_account(op.from);
        format_account(op.to);
    }

    auto state_writer::operator()(const transfer_from_savings_operation& op) -> result_type {
        try {
            format_account(op.from);

            auto& swo = db_.get_savings_withdraw(op.from, op.request_id);

            auto oid = std::string(op.from).append("/").append(std::to_string(op.request_id));
            auto oid_hash = hash_oid(oid);

            auto doc = create_document("savings_withdraw_object", "_id", oid_hash);

            auto& body = doc.doc;

            body << "$set" << open_document;

            format_oid(body, oid); 

            format_value(body, "removed", false);
            format_value(body, "from", op.from);
            format_value(body, "to", op.to);
#ifndef IS_LOW_MEM
            format_value(body, "memo", op.memo);
#endif
            format_value(body, "request_id", swo.request_id);
            format_value(body, "amount", op.amount);
            format_value(body, "complete", swo.complete);

            body << close_document;

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) {
            //
        }
    }

    auto state_writer::operator()(const cancel_transfer_from_savings_operation& op) -> result_type {
        format_account(op.from);
        try {
            db_.get_savings_withdraw(op.from, op.request_id);
        } catch (...) {
            auto oid = std::string(op.from).append("/").append(std::to_string(op.request_id));
            auto oid_hash = hash_oid(oid);

            auto doc = create_removal_document("savings_withdraw_object", "_id", oid_hash);

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
    }

    auto state_writer::operator()(const custom_binary_operation& op) -> result_type {
        
    }

    auto state_writer::operator()(const decline_voting_rights_operation& op) -> result_type {
        try {
            if (op.decline) {
                const auto &account = db_.get_account(op.account);
                const auto &request_idx = db_.get_index<decline_voting_rights_request_index>().indices().get<by_account>();
                auto itr = request_idx.find(account.id);
                if (itr != request_idx.end()) {
                    auto oid = op.account;
                    auto oid_hash = hash_oid(oid);

                    auto doc = create_document("decline_voting_rights_request_object", "_id", oid_hash);

                    auto &body = doc.doc;

                    body << "$set" << open_document;

                    format_oid(body, oid);

                    format_value(body, "account", hash_oid(account.name));
                    format_value(body, "effective_date", itr->effective_date);

                    body << close_document;

                    bmi_insert_or_replace(all_docs, std::move(doc));
                }
            } else {
                auto oid = op.account;
                auto oid_hash = hash_oid(oid);

                auto doc = create_removal_document("decline_voting_rights_request_object", "_id", oid_hash);

                bmi_insert_or_replace(all_docs, std::move(doc));
            }
        } catch (...) {
        }
    }

    auto state_writer::operator()(const reset_account_operation& op) -> result_type {
        format_account_authority(op.account_to_reset);
    }

    auto state_writer::operator()(const set_reset_account_operation& op) -> result_type {
        format_account(op.account);
    }

    auto state_writer::operator()(const delegate_vesting_shares_operation& op) -> result_type {
        format_account(op.delegator);
        format_account(op.delegatee);
        auto delegation = db_.find<vesting_delegation_object, by_delegation>(std::make_tuple(op.delegator, op.delegatee));
        if (delegation != nullptr) {
            format_vesting_delegation_object(*delegation);
        } else {
            auto oid = std::string(op.delegator).append("/").append(std::string(op.delegatee));
            auto oid_hash = hash_oid(oid);
            auto doc = create_removal_document("vesting_delegation_object", "_id", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc));
        }
    }

    auto state_writer::operator()(const proposal_create_operation& op) -> result_type {
        try {
            auto& proposal = db_.get_proposal(op.author, op.title);
            format_proposal(proposal);

            flat_set<account_name_type> required_total;

            required_total.insert(proposal.required_owner_approvals.begin(), proposal.required_owner_approvals.end());
            required_total.insert(proposal.required_active_approvals.begin(), proposal.required_active_approvals.end());
            required_total.insert(proposal.required_posting_approvals.begin(), proposal.required_posting_approvals.end());

            for (const auto& account: required_total) {
                auto& required_approval = db_.get<required_approval_object, golos::chain::by_account>(
                    boost::make_tuple(account, proposal.id));
                format_required_approval(required_approval, op.author, op.title);
            }
        } catch (...) {

        }
    }

    auto state_writer::operator()(const proposal_update_operation& op) -> result_type {
        try {
            auto& proposal = db_.get_proposal(op.author, op.title);
            format_proposal(proposal);
        } catch (...) {
            auto oid = op.author + "/" + op.title;
            auto oid_hash = hash_oid(oid);
            auto doc = create_removal_document("proposal_object", "_id", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc));
            
            auto doc2 = create_removal_document("required_approval_object", "proposal", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc2));
        }
    }

    auto state_writer::operator()(const proposal_delete_operation& op) -> result_type {
        try {
            db_.get_proposal(op.author, op.title);
        } catch (...) {
            auto oid = op.author + "/" + op.title;
            auto oid_hash = hash_oid(oid);
            auto doc = create_removal_document("proposal_object", "_id", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc));
            
            auto doc2 = create_removal_document("required_approval_object", "proposal", oid_hash);
            bmi_insert_or_replace(all_docs, std::move(doc2));
        }
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
            auto comment_oid_hash = hash_oid(comment_oid);

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
            auto vote_oid_hash = hash_oid(vote_oid);

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
            auto comment_oid_hash = hash_oid(comment_oid);

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
            auto benefactor_oid_hash = hash_oid(benefactor_oid);

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
        try {
            db_.get_account(op.owner); // verify owner exists
            const auto &witness = db_.get_witness(op.owner);
            format_witness(witness);
        }
        catch (...) {
            // ilog("Unknown exception during formatting witness.");
        }
    }

    void state_writer::write_global_property_object(const dynamic_global_property_object& dgpo,
        const signed_block& current_block, bool history) {
        try {
            std::string oid;
            if (history) {
                oid = current_block.timestamp;
            } else {
                oid = MONGO_ID_SINGLE;
            }
            auto oid_hash = hash_oid(oid);

            auto doc = create_document(std::string("dynamic_global_property_object") + 
                 (history ? "_history" : ""), "_id", oid_hash);
            auto& body = doc.doc;

            if (!history) {
                body << "$set" << open_document;
            }

            format_oid(body, oid);

            if (history) {
                format_value(body, "timestamp", current_block.timestamp);
            }
            format_value(body, "head_block_number", dgpo.head_block_number);
            format_value(body, "head_block_id", dgpo.head_block_id.str());
            format_value(body, "time", dgpo.time);
            format_value(body, "current_witness", dgpo.current_witness);

            format_value(body, "total_pow", dgpo.total_pow);

            format_value(body, "num_pow_witnesses", dgpo.num_pow_witnesses);

            format_value(body, "virtual_supply", dgpo.virtual_supply);
            format_value(body, "current_supply", dgpo.current_supply);
            format_value(body, "confidential_supply", dgpo.confidential_supply);
            format_value(body, "current_sbd_supply", dgpo.current_sbd_supply);
            format_value(body, "confidential_sbd_supply", dgpo.confidential_sbd_supply);
            format_value(body, "total_vesting_fund_steem", dgpo.total_vesting_fund_steem);
            format_value(body, "total_vesting_shares", dgpo.total_vesting_shares);
            format_value(body, "total_reward_fund_steem", dgpo.total_reward_fund_steem);
            format_value(body, "total_reward_shares2", dgpo.total_reward_shares2);

            format_value(body, "sbd_interest_rate", dgpo.sbd_interest_rate);

            format_value(body, "sbd_print_rate", dgpo.sbd_print_rate);

            format_value(body, "average_block_size", dgpo.average_block_size);

            format_value(body, "maximum_block_size", dgpo.maximum_block_size);

            format_value(body, "current_aslot", dgpo.current_aslot);

            format_value(body, "recent_slots_filled", dgpo.head_block_number);
            format_value(body, "participation_count", dgpo.participation_count);

            format_value(body, "last_irreversible_block_num", dgpo.last_irreversible_block_num);

            format_value(body, "max_virtual_bandwidth", dgpo.max_virtual_bandwidth);

            format_value(body, "current_reserve_ratio", dgpo.current_reserve_ratio);

            format_value(body, "vote_regeneration_per_day", dgpo.vote_regeneration_per_day);

            if (!history) {
                body << close_document;
            }

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) {
            // ilog("Unknown exception during writing global property object.");
        }
    }

    void state_writer::write_witness_schedule_object(const witness_schedule_object& wso,
        const signed_block& current_block, bool history) {
        try {
            std::string oid;
            if (history) {
                oid = current_block.timestamp;
            } else {
                oid = MONGO_ID_SINGLE;
            }
            auto oid_hash = hash_oid(oid);

            auto doc = create_document(std::string("witness_schedule_object") + 
                 (history ? "_history" : ""), "_id", oid_hash);
            auto& body = doc.doc;

            if (!history) {
                body << "$set" << open_document;
            }

            format_oid(body, oid);

            if (history) {
                format_value(body, "timestamp", current_block.timestamp);
            }
            format_value(body, "current_virtual_time", wso.current_virtual_time);
            format_value(body, "next_shuffle_block_num", wso.next_shuffle_block_num);
            format_array_value(body, "current_shuffled_witnesses", wso.current_shuffled_witnesses,
                [] (const account_name_type& item) -> std::string { return std::string(item); });
            format_value(body, "num_scheduled_witnesses", wso.num_scheduled_witnesses);
            format_value(body, "top19_weight", wso.current_virtual_time);
            format_value(body, "timeshare_weight", wso.current_virtual_time);
            format_value(body, "miner_weight", wso.current_virtual_time);
            format_value(body, "witness_pay_normalization_factor", wso.current_virtual_time);
            //format_value(body, "median_props", ... // Skipped because it is still empty
            format_value(body, "majority_version", wso.current_virtual_time);

            if (!history) {
                body << close_document;
            }

            bmi_insert_or_replace(all_docs, std::move(doc));
        }
        catch (...) {
            // ilog("Unknown exception during writing witness schedule object.");
        }
    }

}}}