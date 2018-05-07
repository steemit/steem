#include <golos/protocol/proposal_operations.hpp>
#include <golos/protocol/operations.hpp>
#include <golos/protocol/types.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/time.hpp>

namespace golos { namespace protocol {

    inline void validate_account_name(const string& name) {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
    }

    void proposal_create_operation::validate() const {
        validate_account_name(author);

        FC_ASSERT(!title.empty(), "Title is empty");
        FC_ASSERT(fc::is_utf8(title), "Title not formatted in UTF8");

        FC_ASSERT(!proposed_operations.empty());
        for (const auto& op : proposed_operations) {
            operation_validate(op.op);
        }

        if (memo.size() > 0) {
            FC_ASSERT(fc::is_utf8(memo), "Memo not formatted in UTF8");
        }
    }

    void proposal_update_operation::validate() const {
        validate_account_name(author);

        FC_ASSERT(!title.empty(), "Title is empty");
        FC_ASSERT(fc::is_utf8(title), "Title not formatted in UTF8");

        FC_ASSERT(
            !(active_approvals_to_add.empty() && active_approvals_to_remove.empty() &&
              owner_approvals_to_add.empty() && owner_approvals_to_remove.empty() &&
              posting_approvals_to_add.empty() && posting_approvals_to_remove.empty() &&
              key_approvals_to_add.empty() && key_approvals_to_remove.empty()));

        auto validate = [&](const auto& to_add, const auto& to_remove) {
            for (const auto& a: to_add) {
                FC_ASSERT(
                    to_remove.find(a) == to_remove.end(),
                    "Cannot add and remove approval at the same time.");
            }
        };

        validate(active_approvals_to_add, active_approvals_to_remove);
        validate(owner_approvals_to_add, owner_approvals_to_remove);
        validate(posting_approvals_to_add, posting_approvals_to_remove);
        validate(key_approvals_to_add, key_approvals_to_remove);
    }

    void proposal_update_operation::get_required_authorities(std::vector<authority>& o) const {
        authority auth;
        for (const auto& k: key_approvals_to_add) {
            auth.key_auths[k] = 1;
        }
        for (const auto& k: key_approvals_to_remove) {
            auth.key_auths[k] = 1;
        }
        auth.weight_threshold = auth.key_auths.size();

        if (auth.key_auths.size() > 0) {
            o.emplace_back(std::move(auth));
        }
    }

    void proposal_update_operation::get_required_active_authorities(flat_set<account_name_type>& a) const {
        a.insert(active_approvals_to_add.begin(), active_approvals_to_add.end());
        a.insert(active_approvals_to_remove.begin(), active_approvals_to_remove.end());
    }

    void proposal_update_operation::get_required_owner_authorities(flat_set<account_name_type>& a) const {
        a.insert(owner_approvals_to_add.begin(), owner_approvals_to_add.end());
        a.insert(owner_approvals_to_remove.begin(), owner_approvals_to_remove.end());
    }

    void proposal_update_operation::get_required_posting_authorities(flat_set<account_name_type>& a) const {
        a.insert(posting_approvals_to_add.begin(), posting_approvals_to_add.end());
        a.insert(posting_approvals_to_remove.begin(), posting_approvals_to_remove.end());
    }

    void proposal_delete_operation::validate() const {
        validate_account_name(author);
        FC_ASSERT(!title.empty(), "Title is empty");
        FC_ASSERT(fc::is_utf8(title), "Title not formatted in UTF8");
    }

} } // golos::chain