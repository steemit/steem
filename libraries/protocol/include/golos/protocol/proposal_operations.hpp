#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/base.hpp>

namespace golos { namespace protocol {
    /**
     * @defgroup proposed_transactions  The Transaction Proposal Protocol
     * @ingroup operations
     *
     * Golos allows users to propose a transaction which requires approval of multiple accounts in order to execute.
     * The user proposes a transaction using proposal_create_operation, then signatory accounts use
     * proposal_update_operations to add or remove their approvals from this operation. When a sufficient number of
     * approvals have been granted, the operations in the proposal are used to create a virtual transaction which is
     * subsequently evaluated. Even if the transaction fails, the proposal will be kept until the expiration time, at
     * which point, if sufficient approval is granted, the transaction will be evaluated a final time. This allows
     * transactions which will not execute successfully until a given time to still be executed through the proposal
     * mechanism. The first time the proposed transaction succeeds, the proposal will be regarded as resolved, and all
     * future updates will be invalid.
     *
     * The proposal system allows for arbitrarily complex or recursively nested authorities. If a recursive authority
     * (i.e. an authority which requires approval of 'nested' authorities on other accounts) is required for a
     * proposal, then a second proposal can be used to grant the nested authority's approval. That is, a second
     * proposal can be created which, when sufficiently approved, adds the approval of a nested authority to the first
     * proposal. This multiple-proposal scheme can be used to acquire approval for an arbitrarily deep authority tree.
     *
     * Note that at any time, a proposal can be approved in a single transaction if sufficient signatures are available
     * on the proposal_update_operation, as long as the authority tree to approve the proposal does not exceed the
     * maximum recursion depth. In practice, however, it is easier to use proposals to acquire all approvals, as this
     * leverages on-chain notification of all relevant parties that their approval is required. Off-chain
     * multi-signature approval requires some off-chain mechanism for acquiring several signatures on a single
     * transaction. This off-chain synchronization can be avoided using proposals.
     * @{
     *
     * operaion_wrapper is used to get around the circular definition of operation and proposals that contain them.
     */
    struct operation_wrapper;

    /**
     * @brief The proposal_create_operation creates a transaction proposal, for use in multi-sig scenarios
     * @ingroup operations
     *
     * Creates a transaction proposal. The operations which compose the transaction are listed in order in proposed_ops,
     * and expiration_time specifies the time by which the proposal must be accepted or it will fail permanently. The
     * expiration_time cannot be farther in the future than the maximum expiration time set in the global properties
     * object.
     */
    struct proposal_create_operation : public base_operation {
        account_name_type author;
        std::string title;
        std::string memo;
        std::vector<operation_wrapper> proposed_operations;
        time_point_sec expiration_time;
        optional<time_point_sec> review_period_time;
        extensions_type extensions;

        void validate() const;

        void get_required_active_authorities(flat_set<account_name_type>& a) const {
            a.insert(author);
        }
    };

    /**
     * @brief The proposal_update_operation updates an existing transaction proposal
     * @ingroup operations
     *
     * This operation allows accounts to add or revoke approval of a proposed transaction. Signatures sufficient to
     * satisfy the authority of each account in approvals are required on the transaction containing this operation.
     *
     * If an account with a multi-signature authority is listed in approvals_to_add or approvals_to_remove, either all
     * required signatures to satisfy that account's authority must be provided in the transaction containing this
     * operation, or a secondary proposal must be created which contains this operation.
     *
     * NOTE: If the proposal requires only an account's active authority, the account must not update adding its owner
     * authority's approval. This is considered an error. An owner approval may only be added if the proposal requires
     * the owner's authority.
     *
     * If an account's owner and active authority are both required, only the owner authority may approve. An attempt to
     * add or remove active authority approval to such a proposal will fail.
     */
    struct proposal_update_operation : public base_operation {
        account_name_type author;
        std::string title;
        flat_set<account_name_type> active_approvals_to_add;
        flat_set<account_name_type> active_approvals_to_remove;
        flat_set<account_name_type> owner_approvals_to_add;
        flat_set<account_name_type> owner_approvals_to_remove;
        flat_set<account_name_type> posting_approvals_to_add;
        flat_set<account_name_type> posting_approvals_to_remove;
        flat_set<public_key_type> key_approvals_to_add;
        flat_set<public_key_type> key_approvals_to_remove;
        extensions_type extensions;

        void validate() const;

        void get_required_authorities(std::vector<authority>&) const;

        void get_required_active_authorities(flat_set<account_name_type>&) const;

        void get_required_owner_authorities(flat_set<account_name_type>&) const;

        void get_required_posting_authorities(flat_set<account_name_type>&) const;
    };

    /**
     * @brief The proposal_delete_operation deletes an existing transaction proposal
     * @ingroup operations
     *
     * This operation allows the early veto of a proposed transaction. It may be used by any account which is a required
     * authority on the proposed transaction, when that account's holder feels the proposal is ill-advised and he decides
     * he will never approve of it and wishes to put an end to all discussion of the issue. Because he is a required
     * authority, he could simply refuse to add his approval, but this would leave the topic open for debate until the
     * proposal expires. Using this operation, he can prevent any further breath from being wasted on such an absurd
     * proposal.
     */
    struct proposal_delete_operation : public base_operation {
        account_name_type author;
        std::string title;
        account_name_type requester;
        extensions_type extensions;

        void validate() const;

        void get_required_active_authorities(flat_set<account_name_type>& a) const {
            a.insert(requester);
        }
    };
    ///@}
} } // golos::chain

FC_REFLECT(
    (golos::protocol::proposal_create_operation),
    (author)(title)(memo)(expiration_time)(proposed_operations)(review_period_time)(extensions))

FC_REFLECT(
    (golos::protocol::proposal_update_operation),
    (author)(title)(active_approvals_to_add)(active_approvals_to_remove)(owner_approvals_to_add)
    (owner_approvals_to_remove)(posting_approvals_to_add)(posting_approvals_to_remove)
    (key_approvals_to_add)(key_approvals_to_remove)(extensions))

FC_REFLECT(
    (golos::protocol::proposal_delete_operation),
    (author)(title)(requester)(extensions))
