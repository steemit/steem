#include <fc/io/datastream.hpp>

#include <golos/protocol/proposal_operations.hpp>
#include <golos/chain/steem_evaluator.hpp>
#include <golos/chain/database.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/proposal_object.hpp>

namespace golos { namespace chain {
    namespace {
        template <typename F, typename S>
        void remove_existing(F& first, const S& second) {
            auto src = std::move(first);

            std::set_difference(
                src.begin(), src.end(),
                second.begin(), second.end(),
                std::inserter(first, first.begin()));
        }
    }

    void proposal_create_evaluator::do_apply(const proposal_create_operation& o) { try {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_18__542, "Proposal transaction creating"); // remove after hf

        FC_ASSERT(nullptr == db().find_proposal(o.author, o.title), "Proposal already exists.");

        const auto now = db().head_block_time();
        FC_ASSERT(
            o.expiration_time > now,
            "Proposal has already expired on creation.");
        FC_ASSERT(
            o.expiration_time <= now + STEEMIT_MAX_PROPOSAL_LIFETIME_SEC,
            "Proposal expiration time is too far in the future.");
        FC_ASSERT(
            !o.review_period_time || *o.review_period_time > now,
            "Proposal review period has expired on creation.");
        FC_ASSERT(
            !o.review_period_time || *o.review_period_time < o.expiration_time,
            "Proposal review period must be less than its overall lifetime.");

        //Populate the required approval sets
        flat_set<account_name_type> required_owner;
        flat_set<account_name_type> required_active;
        flat_set<account_name_type> required_posting;
        flat_set<account_name_type> required_total;
        std::vector<authority> other;

        for (const auto& op : o.proposed_operations) {
            operation_get_required_authorities(op.op, required_active, required_owner, required_posting, other);
        }
        FC_ASSERT(other.size() == 0); // TODO: what about other???

        // All accounts which must provide both owner and active authority should be omitted from
        // the active authority set. Owner authority approval implies active authority approval.
        required_total.insert(required_owner.begin(), required_owner.end());
        remove_existing(required_active, required_total);
        required_total.insert(required_active.begin(), required_active.end());

        // For more information, see sign_state.cpp
        FC_ASSERT(
            required_posting.empty() != required_total.empty(),
            "Can't combine operations required posting authority and active or owner authority");
        required_total.insert(required_posting.begin(), required_posting.end());

        transaction trx;
        for (const auto& op : o.proposed_operations) {
            trx.operations.push_back(op.op);
        }
        trx.validate();

        auto ops_size = fc::raw::pack_size(trx.operations);

        const auto& proposal = db().create<proposal_object>([&](proposal_object& p){
            p.author = o.author;
            from_string(p.title, o.title);
            from_string(p.memo, o.memo);
            p.expiration_time = o.expiration_time;
            if (o.review_period_time) {
                p.review_period_time = o.review_period_time;
            }
            p.proposed_operations.resize(ops_size);
            fc::datastream<char *> ds(p.proposed_operations.data(), ops_size);
            fc::raw::pack(ds, trx.operations);

            p.required_active_approvals.insert(required_active.begin(), required_active.end());
            p.required_owner_approvals.insert(required_owner.begin(), required_owner.end());
            p.required_posting_approvals.insert(required_posting.begin(), required_posting.end());
        });
    } FC_CAPTURE_AND_RETHROW((o)) }

    void proposal_update_evaluator::do_apply(const proposal_update_operation& o) { try {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_18__542, "Proposal transaction updating"); // remove after hf
        auto& proposal = db().get_proposal(o.author, o.title);
        const auto now = db().head_block_time();

        if (proposal.review_period_time && now >= *proposal.review_period_time) {
            FC_ASSERT(
                o.active_approvals_to_add.empty() &&
                o.owner_approvals_to_add.empty() &&
                o.posting_approvals_to_add.empty() &&
                o.key_approvals_to_add.empty(),
                "This proposal is in its review period. No new approvals may be added.");
        }

        auto check_existing = [&](const auto& to_remove, const auto& dst) {
            for (const auto& a: to_remove) {
                FC_ASSERT(dst.find(a) != dst.end(), "Can't remove the non existing approval", ("id", a));
            }
        };

        check_existing(o.active_approvals_to_remove, proposal.available_active_approvals);
        check_existing(o.owner_approvals_to_remove, proposal.available_owner_approvals);
        check_existing(o.posting_approvals_to_remove, proposal.available_posting_approvals);
        check_existing(o.key_approvals_to_remove, proposal.available_key_approvals);

        db().modify(proposal, [&](proposal_object &p){
            p.available_active_approvals.insert(o.active_approvals_to_add.begin(), o.active_approvals_to_add.end());
            p.available_owner_approvals.insert(o.owner_approvals_to_add.begin(), o.owner_approvals_to_add.end());
            p.available_posting_approvals.insert(o.posting_approvals_to_add.begin(), o.posting_approvals_to_add.end());
            p.available_key_approvals.insert(o.key_approvals_to_add.begin(), o.key_approvals_to_add.end());

            remove_existing(p.available_active_approvals, o.active_approvals_to_remove);
            remove_existing(p.available_owner_approvals, o.owner_approvals_to_remove);
            remove_existing(p.available_posting_approvals, o.posting_approvals_to_remove);
            remove_existing(p.available_key_approvals, o.key_approvals_to_remove);
        });

        if (proposal.review_period_time) {
            // if no ability to add an approval, there is no reason to keep the proposal
            if (now >= *proposal.review_period_time &&
                proposal.available_active_approvals.empty() &&
                proposal.available_owner_approvals.empty() &&
                proposal.available_posting_approvals.empty() &&
                proposal.available_key_approvals.empty()
            ) {
                db().remove(proposal);
            }
            return;
        }

        if (db().is_authorized_to_execute(proposal)) {
            // All required approvals are satisfied. Execute!
            try {
                db().push_proposal(proposal);
            } catch (fc::exception &e) {
                wlog(
                    "Proposed transaction ${author}::${title} failed to apply once approved with exception:\n"
                    "----\n${reason}\n"
                    "----\nWill try again when it expires.",
                    ("author", o.author)("title", o.title)("reason", e.to_detail_string()));
            }
        }
    } FC_CAPTURE_AND_RETHROW((o)) }

    void proposal_delete_evaluator::do_apply(const proposal_delete_operation& o) { try {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_18__542, "Proposal transaction deleting"); // remove after hf
        const auto& proposal = db().get_proposal(o.author, o.title);

        FC_ASSERT(
            proposal.author == o.requester ||
            proposal.required_active_approvals.count(o.requester) ||
            proposal.required_owner_approvals.count(o.requester) ||
            proposal.required_posting_approvals.count(o.requester),
            "Provided authority is not authoritative for this proposal.",
            ("author", o.author)("title", o.title)("requester", o.requester));

        db().remove(proposal);

    } FC_CAPTURE_AND_RETHROW((o)) }

} } // golos::chain
