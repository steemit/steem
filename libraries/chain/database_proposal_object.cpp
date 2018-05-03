#include <golos/chain/database.hpp>
#include <golos/chain/proposal_object.hpp>
#include <golos/chain/account_object.hpp>

namespace golos { namespace chain {

    const proposal_object& database::get_proposal(
        const account_name_type& author,
        const std::string& title
    ) const { try {
        return get<proposal_object, by_account>(std::make_tuple(author, title));
    } FC_CAPTURE_AND_RETHROW((author)(title)) }

    const proposal_object *database::find_proposal(
        const account_name_type& author,
        const std::string& title
    ) const {
        return find<proposal_object, by_account>(std::make_tuple(author, title));
    }

    bool database::is_authorized_to_execute(const proposal_object& proposal) const {
        static const chain_id_type chain_id = STEEMIT_CHAIN_ID;

        auto get_active = [&](const string& name) {
            return authority(get<account_authority_object, by_account>(name).active);
        };

        auto get_owner = [&](const string& name) {
            return authority(get<account_authority_object, by_account>(name).owner);
        };

        auto get_posting = [&](const string& name) {
            return authority(get<account_authority_object, by_account>(name).posting);
        };

        return proposal.is_authorized_to_execute(
            chain_id, get_active, get_owner, get_posting, STEEMIT_MAX_SIG_CHECK_DEPTH);
    }

    void database::push_proposal(const proposal_object& proposal) { try {
        auto ops = proposal.operations();
        for (auto& op : ops) {
            apply_operation(op);
        }
        remove(proposal);
    } FC_CAPTURE_AND_RETHROW((proposal.author)(proposal.title)) }

    void database::clear_expired_proposals() {
        const auto& proposal_expiration_index = get_index<proposal_index>().indices().get<by_expiration>();
        const auto now = head_block_time();

        while (!proposal_expiration_index.empty() && proposal_expiration_index.begin()->expiration_time <= now) {
            const proposal_object& proposal = *proposal_expiration_index.begin();

            try {
                if (is_authorized_to_execute(proposal)) {
                    push_proposal(proposal);
                    continue;
                }
            } catch (const fc::exception& e) {
                elog(
                    "Failed to apply proposed transaction on its expiration. "
                    "Deleting it.\n${author}::${title}\n${error}",
                    ("author", proposal.author)("title", proposal.title)("error", e.to_detail_string()));
            }
            remove(proposal);
        }
    }

} } // golos::chain