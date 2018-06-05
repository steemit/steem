#include <golos/plugins/mongo_db/mongo_db_operations.hpp>

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/value_context.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <appbase/plugin.hpp>


namespace golos {
namespace plugins {
namespace mongo_db {

    using bsoncxx::builder::stream::array;
    using bsoncxx::builder::stream::document;

    document format_authority(const authority &auth) {
        array account_auths_arr;
        for (const auto &iter : auth.account_auths) {
            document temp;
            temp << "public_key_type" << (std::string) iter.first
                 << "weight_type" << iter.second;
            account_auths_arr << temp;
        }
        array key_auths_arr;
        for (auto &iter : auth.key_auths) {
            document temp;
            temp << "public_key_type" << (std::string) iter.first
                 << "weight_type" << iter.second;
            key_auths_arr << temp;
        }

        document authority_doc;
        authority_doc << "owner" << auth.owner;
        format_value(authority_doc, "weight_threshold", auth.weight_threshold);
        authority_doc << "account_auths" << account_auths_arr;
        authority_doc << "key_auths" << key_auths_arr;

        return authority_doc;
    }

    void format_authority(document &doc, const std::string &name, const authority &auth) {
        doc << name << format_authority(auth);
    }

    std::string to_string(const signature_type& signature) {
        std::string retVal;

        for (auto& iter : signature) {
            retVal += iter;
        }
        return retVal;
    }

    void format_chain_properties_17(document& doc, const chain_properties_17& props) {
        format_value(doc, "account_creation_fee", props.account_creation_fee);
        format_value(doc, "maximum_block_size", props.maximum_block_size);
        format_value(doc, "sbd_interest_rate", props.sbd_interest_rate);
    }

    void format_chain_properties_v(document& doc, const versioned_chain_properties& props) {
        //TODO
    }

    /////////////////////////////////////////////////

    operation_writer::operation_writer() {
    }

    auto operation_writer::operator()(const vote_operation& op) -> result_type {
        result_type body;

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);
        format_value(body, "voter", op.voter);
        format_value(body, "weight", op.weight);

        return body;
    }

    auto operation_writer::operator()(const comment_operation& op) -> result_type {
        result_type body;

        format_value(body, "parent_author", op.parent_author);
        format_value(body, "parent_permlink", op.parent_permlink);
        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);
        format_value(body, "title", op.title);
        format_value(body, "body", op.body);
        format_value(body, "json_metadata", op.json_metadata);

        return body;
    }

    auto operation_writer::operator()(const transfer_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "amount", op.amount);
        format_value(body, "memo", op.memo);

        return body;
    }

    auto operation_writer::operator()(const transfer_to_vesting_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "amount", op.amount);

        return body;
    }

    auto operation_writer::operator()(const withdraw_vesting_operation& op) -> result_type {
        result_type body;

        format_value(body, "account", op.account);
        format_value(body, "vesting_shares", op.vesting_shares);

        return body;
    }

    auto operation_writer::operator()(const limit_order_create_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "orderid", op.orderid);
        format_value(body, "amount_to_sell", op.amount_to_sell);
        format_value(body, "min_to_receive", op.min_to_receive);
        format_value(body, "expiration", op.expiration);

        return body;
    }

    auto operation_writer::operator()(const limit_order_cancel_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "orderid", op.orderid);

        return body;
    }

    auto operation_writer::operator()(const feed_publish_operation& op) -> result_type {
        result_type body;

        format_value(body, "publisher", op.publisher);
        format_value(body, "exchange_rate", op.exchange_rate.to_real());

        return body;
    }

    auto operation_writer::operator()(const convert_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "requestid", op.requestid);
        format_value(body, "amount", op.amount);

        return body;
    }

    auto operation_writer::operator()(const account_create_operation& op) -> result_type {
        result_type body;

        format_value(body, "fee", op.fee);
        format_value(body, "creator", op.creator);
        format_value(body, "new_account_name", op.new_account_name);
        format_authority(body, "owner", op.owner);
        format_value(body, "json_metadata", op.json_metadata);
        format_value(body, "memo_key", (std::string)op.memo_key);
        format_authority(body, "posting", op.posting);

        return body;
    }

    auto operation_writer::operator()(const account_update_operation& op) -> result_type {
        result_type body;

        if (op.owner) {
            format_authority(body, "owner", *op.owner);
        }

        document posting_owner_doc;
        if (op.posting) {
            format_authority(body, "posting", *op.posting);
        }

        document active_owner_doc;
        if (op.active) {
            format_authority(body, "active", *op.active);
        }

        format_value(body, "account", op.account);
        format_value(body, "json_metadata", op.json_metadata);
        format_value(body, "memo_key", (std::string)op.memo_key);

        return body;
    }

    auto operation_writer::operator()(const witness_update_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "fee", op.fee);
        format_value(body, "url", op.url);
        format_value(body, "block_signing_key", (std::string)op.block_signing_key);
        format_chain_properties_17(body, op.props);

        return body;
    }

    auto operation_writer::operator()(const account_witness_vote_operation& op) -> result_type {
        result_type body;

        format_value(body, "account", op.account);
        format_value(body, "witness", op.witness);
        format_value(body, "approve", (op.approve ? std::string("true") : std::string("false")));

        return body;
    }

    auto operation_writer::operator()(const account_witness_proxy_operation& op) -> result_type {
        result_type body;

        format_value(body, "account", op.account);
        format_value(body, "proxy", op.proxy);

        return body;
    }

    auto operation_writer::operator()(const pow_operation& op) -> result_type {
        result_type body;

        format_value(body, "worker", (std::string)op.work.worker);
        format_value(body, "input", op.work.input.str());
        format_value(body, "signature", to_string(op.work.signature));
        format_value(body, "work", op.work.work.str());

        format_value(body, "block_id", op.block_id.str());
        format_value(body, "worker_account", op.worker_account);
        format_value(body, "nonce", op.nonce);

        format_chain_properties_17(body, op.props);

        return body;
    }

    auto operation_writer::operator()(const custom_operation& op) -> result_type {
        result_type body;

        format_value(body, "id", op.id);
        if (!op.required_auths.empty()) {
            array auths;
            for (auto& iter : op.required_auths) {
                auths << iter;
            }
            body << "required_auths" << auths;
        }

        return body;
    }

    auto operation_writer::operator()(const report_over_production_operation& op) -> result_type {
        result_type body;

        document doc1;
        format_value(doc1, "id", op.first_block.id().str());
        format_value(doc1, "timestamp", op.first_block.timestamp);
        format_value(doc1, "witness", op.first_block.witness);

        document doc2;
        format_value(doc2, "id", op.second_block.id().str());
        format_value(doc2, "timestamp", op.first_block.timestamp);
        format_value(doc2, "witness", op.second_block.witness);

        format_value(body, "reporter", op.reporter);

        body << "first_block" << doc1;
        body << "second_block" << doc2;

        return body;
    }

    auto operation_writer::operator()(const delete_comment_operation& op) -> result_type {
        result_type body;

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);

        return body;
    }

    auto operation_writer::operator()(const custom_json_operation& op) -> result_type {
        result_type body;

        format_value(body, "id", op.id);
        format_value(body, "json", op.json);

        return body;
    }

    auto operation_writer::operator()(const comment_options_operation& op) -> result_type {
        result_type body;

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);
        format_value(body, "max_accepted_payout", op.max_accepted_payout);
        format_value(body, "percent_steem_dollars", op.percent_steem_dollars);
        format_value(body, "allow_votes", op.allow_votes);
        format_value(body, "allow_curation_rewards", op.allow_curation_rewards);
        // comment_options_extensions_type extensions;

        return body;
    }

    auto operation_writer::operator()(const set_withdraw_vesting_route_operation& op) -> result_type {
        result_type body;

        format_value(body, "from_account", op.from_account);
        format_value(body, "to_account", op.to_account);
        format_value(body, "percent", op.percent);
        format_value(body, "auto_vest", op.auto_vest);

        return body;
    }

    auto operation_writer::operator()(const limit_order_create2_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "orderid", op.orderid);
        format_value(body, "amount_to_sell", op.amount_to_sell);
        format_value(body, "fill_or_kill", op.fill_or_kill);
        format_value(body, "exchange_rate", op.exchange_rate.to_real());
        format_value(body, "expiration", op.expiration);

        return body;
    }

    auto operation_writer::operator()(const challenge_authority_operation& op) -> result_type {
        result_type body;

        format_value(body, "challenger", op.challenger);
        format_value(body, "challenged", op.challenged);
        format_value(body, "require_owner", op.require_owner);

        return body;
    }

    auto operation_writer::operator()(const prove_authority_operation& op) -> result_type {
        result_type body;

        format_value(body, "challenged", op.challenged);
        format_value(body, "require_owner", op.require_owner);

        return body;
    }

    auto operation_writer::operator()(const request_account_recovery_operation& op) -> result_type {
        result_type body;

        format_value(body, "recovery_account", op.recovery_account);
        format_value(body, "account_to_recover", op.account_to_recover);
        format_authority(body, "new_owner_authority", op.new_owner_authority);

        return body;
    }

    auto operation_writer::operator()(const recover_account_operation& op) -> result_type {
        result_type body;

        format_value(body, "account_to_recover", op.account_to_recover);
        format_authority(body, "new_owner_authority", op.new_owner_authority);
        format_authority(body, "recent_owner_authority", op.recent_owner_authority);

        return body;
    }

    auto operation_writer::operator()(const change_recovery_account_operation& op) -> result_type {
        result_type body;

        format_value(body, "account_to_recover", op.account_to_recover);
        format_value(body, "new_recovery_account", op.new_recovery_account);

        return body;
    }

    auto operation_writer::operator()(const escrow_transfer_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "agent", op.agent);
        format_value(body, "escrow_id", op.escrow_id);

        format_value(body, "sbd_amount", op.sbd_amount);
        format_value(body, "steem_amount ", op.steem_amount);
        format_value(body, "fee", op.fee);
        format_value(body, "json_meta", op.json_meta);

        return body;
    }

    auto operation_writer::operator()(const escrow_dispute_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "agent", op.agent);
        format_value(body, "who", op.who);
        format_value(body, "escrow_id", op.escrow_id);

        return body;
    }

    auto operation_writer::operator()(const escrow_release_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "agent", op.agent);
        format_value(body, "who", op.who);
        format_value(body, "receiver", op.receiver);
        format_value(body, "escrow_id", op.escrow_id);

        format_value(body, "sbd_amount", op.sbd_amount);
        format_value(body, "steem_amount", op.steem_amount);

        return body;
    }

    auto operation_writer::operator()(const pow2_operation& op) -> result_type {
        result_type body;

        format_chain_properties_17(body, op.props);
        if (op.new_owner_key) {
            format_value(body, "new_owner_key", (std::string)(*op.new_owner_key));
        }

        return body;
    }

    auto operation_writer::operator()(const escrow_approve_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "agent", op.agent);
        format_value(body, "who", op.who);
        format_value(body, "escrow_id", op.escrow_id);
        format_value(body, "approve", op.approve);

        return body;
    }

    auto operation_writer::operator()(const transfer_to_savings_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "amount", op.amount);
        format_value(body, "memo", op.memo);

        return body;
    }

    auto operation_writer::operator()(const transfer_from_savings_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "amount", op.amount);
        format_value(body, "memo", op.memo);
        format_value(body, "request_id", op.request_id);

        return body;
    }

    auto operation_writer::operator()(const cancel_transfer_from_savings_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "request_id", op.request_id);

        return body;
    }

    auto operation_writer::operator()(const custom_binary_operation& op) -> result_type {
        result_type body;

        array required_owner_auths_arr;
        for (auto& iter : op.required_owner_auths) {
            required_owner_auths_arr << iter;
        }

        array required_active_auths_arr;
        for (auto& iter : op.required_active_auths) {
            required_active_auths_arr << iter;
        }

        array required_posting_auths_arr;
        for (auto& iter : op.required_posting_auths) {
            required_posting_auths_arr << iter;
        }

        array auths;
        for (auto& iter : op.required_auths) {
            auths << format_authority(iter);
        }

        format_value(body, "id", op.id);
        body << "required_owner_auths" << required_owner_auths_arr;
        body << "required_active_auths" << required_active_auths_arr;
        body << "required_posting_auths" << required_posting_auths_arr;
        body << "required_auths" << auths;

        return body;
    }

    auto operation_writer::operator()(const decline_voting_rights_operation& op) -> result_type {
        result_type body;

        format_value(body, "account", op.account);
        format_value(body, "decline", op.decline);

        return body;
    }

    auto operation_writer::operator()(const reset_account_operation& op) -> result_type {
        result_type body;

        format_value(body, "reset_account", op.reset_account);
        format_value(body, "account_to_reset", op.account_to_reset);
        format_authority(body, "new_owner_authority", op.new_owner_authority);

        return body;
    }

    auto operation_writer::operator()(const set_reset_account_operation& op) -> result_type {
        result_type body;

        format_value(body, "account", op.account);
        format_value(body, "current_reset_account", op.current_reset_account);
        format_value(body, "reset_account", op.reset_account);

        return body;
    }

//
    auto operation_writer::operator()(const delegate_vesting_shares_operation& op) -> result_type {
        result_type body;

        return body;
    }
    auto operation_writer::operator()(const account_create_with_delegation_operation& op) -> result_type {
        result_type body;

        return body;
    }
    auto operation_writer::operator()(const account_metadata_operation& op) -> result_type {
        result_type body;

        return body;
    }
    auto operation_writer::operator()(const proposal_create_operation& op) -> result_type {
        result_type body;

        return body;
    }
    auto operation_writer::operator()(const proposal_update_operation& op) -> result_type {
        result_type body;

        return body;
    }
    auto operation_writer::operator()(const proposal_delete_operation& op) -> result_type {
        result_type body;

        return body;
    }
//
    auto operation_writer::operator()(const fill_convert_request_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "requestid", op.requestid);
        format_value(body, "amount_in", op.amount_in);
        format_value(body, "amount_out", op.amount_out);

        return body;
    }

    auto operation_writer::operator()(const author_reward_operation& op) -> result_type {
        result_type body;

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);
        format_value(body, "sbd_payout", op.sbd_payout);
        format_value(body, "steem_payout", op.steem_payout);
        format_value(body, "vesting_payout", op.vesting_payout);

        return body;
    }

    auto operation_writer::operator()(const curation_reward_operation& op) -> result_type {
        result_type body;

        format_value(body, "curator", op.curator);
        format_value(body, "reward", op.reward);
        format_value(body, "comment_author", op.comment_author);
        format_value(body, "comment_permlink", op.comment_permlink);

        return body;
    }

    auto operation_writer::operator()(const comment_reward_operation& op) -> result_type {
        result_type body;

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);
        format_value(body, "payout", op.payout);

        return body;
    }

    auto operation_writer::operator()(const liquidity_reward_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "payout", op.payout);

        return body;
    }

    auto operation_writer::operator()(const interest_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);
        format_value(body, "interest", op.interest);

        return body;
    }

    auto operation_writer::operator()(const fill_vesting_withdraw_operation& op) -> result_type {
        result_type body;

        format_value(body, "from_account", op.from_account);
        format_value(body, "to_account", op.to_account);
        format_value(body, "withdrawn", op.withdrawn);
        format_value(body, "deposited", op.deposited);

        return body;
    }

    auto operation_writer::operator()(const fill_order_operation& op) -> result_type {
        result_type body;

        format_value(body, "current_owner", op.current_owner);
        format_value(body, "current_orderid", op.current_orderid);
        format_value(body, "current_pays", op.current_pays);
        format_value(body, "open_owner", op.open_owner);
        format_value(body, "open_orderid", op.open_orderid);
        format_value(body, "open_pays", op.open_pays);

        return body;
    }

    auto operation_writer::operator()(const shutdown_witness_operation& op) -> result_type {
        result_type body;

        format_value(body, "owner", op.owner);

        return body;
    }

    auto operation_writer::operator()(const fill_transfer_from_savings_operation& op) -> result_type {
        result_type body;

        format_value(body, "from", op.from);
        format_value(body, "to", op.to);
        format_value(body, "amount", op.amount);
        format_value(body, "request_id", op.request_id);
        format_value(body, "memo", op.memo);

        return body;
    }

    auto operation_writer::operator()(const hardfork_operation& op) -> result_type {
        result_type body;

        format_value(body, "hardfork_id", op.hardfork_id);

        return body;
    }

    auto operation_writer::operator()(const comment_payout_update_operation& op) -> result_type {
        result_type body;

        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);

        return body;
    }

    auto operation_writer::operator()(const comment_benefactor_reward_operation& op) -> result_type {
        result_type body;

        format_value(body, "benefactor", op.benefactor);
        format_value(body, "author", op.author);
        format_value(body, "permlink", op.permlink);
        format_value(body, "reward", op.reward);

        return body;
    }

    auto operation_writer::operator()(const return_vesting_delegation_operation& op) -> result_type {
        result_type body;

        return body;
    }

    auto operation_writer::operator()(const chain_properties_update_operation& op) -> result_type {
        result_type body;
        format_value(body, "owner", op.owner);
        format_chain_properties_v(body, op.props);
        return body;
    }

}}}
