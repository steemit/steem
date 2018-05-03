#include <iterator>

#include <fc/io/datastream.hpp>

#include <golos/chain/proposal_object.hpp>
#include <golos/chain/account_object.hpp>

namespace golos { namespace chain {

    namespace {
        template <typename S> static flat_set<typename S::value_type> copy_to_heap(const S& src) {
            return flat_set<typename S::value_type>(src.begin(), src.end());
        }
    }

    std::vector<protocol::operation> proposal_object::operations() const {
        std::vector<protocol::operation> operations;

        fc::datastream<const char *> ds(proposed_operations.data(), proposed_operations.size());
        fc::raw::unpack(ds, operations);

        return operations;
    }

    bool proposal_object::is_authorized_to_execute(
        const chain_id_type& chain_id,
        const protocol::authority_getter& get_active,
        const protocol::authority_getter& get_owner,
        const protocol::authority_getter& get_posting,
        uint32_t max_recursion
    ) const {

        try {
            auto active_approvals = copy_to_heap(available_active_approvals);
            auto owner_approvals = copy_to_heap(available_owner_approvals);
            auto posting_approvals = copy_to_heap(available_posting_approvals);
            auto key_approvals = copy_to_heap(available_key_approvals);
            auto ops = operations();

            verify_authority(
                ops, key_approvals,
                get_active, get_owner, get_posting, max_recursion, false, /* allow committeee */
                active_approvals, owner_approvals, posting_approvals);
        } catch (const fc::exception& e) {
            //idump((available_active_approvals));
            //wlog((e.to_detail_string()));
            return false;
        }
        return true;
    }

} } // golos::chain