
#include <golos/protocol/transaction.hpp>
#include <golos/protocol/exceptions.hpp>

#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>

namespace golos {
    namespace protocol {

        digest_type signed_transaction::merkle_digest() const {
            digest_type::encoder enc;
            fc::raw::pack(enc, *this);
            return enc.result();
        }

        digest_type transaction::digest() const {
            digest_type::encoder enc;
            fc::raw::pack(enc, *this);
            return enc.result();
        }

        digest_type transaction::sig_digest(const chain_id_type &chain_id) const {
            digest_type::encoder enc;
            fc::raw::pack(enc, chain_id);
            fc::raw::pack(enc, *this);
            return enc.result();
        }

        void transaction::validate() const {
            FC_ASSERT(operations.size() >
                      0, "A transaction must have at least one operation", ("trx", *this));
            for (const auto &op : operations) {
                operation_validate(op);
            }
        }

        golos::protocol::transaction_id_type golos::protocol::transaction::id() const {
            auto h = digest();
            transaction_id_type result;
            memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
            return result;
        }

        const signature_type &golos::protocol::signed_transaction::sign(const private_key_type &key, const chain_id_type &chain_id) {
            digest_type h = sig_digest(chain_id);
            signatures.push_back(key.sign_compact(h));
            return signatures.back();
        }

        signature_type golos::protocol::signed_transaction::sign(const private_key_type &key, const chain_id_type &chain_id) const {
            digest_type::encoder enc;
            fc::raw::pack(enc, chain_id);
            fc::raw::pack(enc, *this);
            return key.sign_compact(enc.result());
        }

        void transaction::set_expiration(fc::time_point_sec expiration_time) {
            expiration = expiration_time;
        }

        void transaction::set_reference_block(const block_id_type &reference_block) {
            ref_block_num = fc::endian_reverse_u32(reference_block._hash[0]);
            ref_block_prefix = reference_block._hash[1];
        }

        void transaction::get_required_authorities(flat_set<account_name_type> &active,
                flat_set<account_name_type> &owner,
                flat_set<account_name_type> &posting,
                vector<authority> &other) const {
            for (const auto &op : operations) {
                operation_get_required_authorities(op, active, owner, posting, other);
            }
        }

        void assert_unused_approvals(sign_state& s) {
            GOLOS_CTOR_ASSERT(
                !s.remove_unused_signatures(),
                tx_irrelevant_sig,
                [&](auto& e) {
                    e.unused_signatures = std::move(s.unused_signatures);
                    e.append_log(GOLOS_ASSERT_MESSAGE("Unnecessary signature(s) detected"));
                });

            GOLOS_CTOR_ASSERT(
                !s.filter_unused_approvals(),
                tx_irrelevant_approval,
                [&](auto& e) {
                    e.unused_approvals = std::move(s.unused_approvals);
                    e.append_log(GOLOS_ASSERT_MESSAGE("Unnecessary approval(s) detected"));
                });
        }

        void verify_authority(
            const std::vector<operation>& ops,
            const fc::flat_set<public_key_type>& sigs,
            const authority_getter& get_active,
            const authority_getter& get_owner,
            const authority_getter& get_posting,
            uint32_t max_recursion_depth,
            bool allow_committe,
            const flat_set<account_name_type>& active_aprovals,
            const flat_set<account_name_type>& owner_approvals,
            const flat_set<account_name_type>& posting_approvals
        ) { try {
            fc::flat_set<account_name_type> required_active;
            fc::flat_set<account_name_type> required_owner;
            fc::flat_set<account_name_type> required_posting;
            std::vector<authority> other;
            fc::flat_set<public_key_type> avail;
            std::vector<account_name_type> missing_accounts;
            std::vector<authority> missing_auths;

            for (const auto& op : ops) {
                operation_get_required_authorities(op, required_active, required_owner, required_posting, other);
            }

            /**
             *  Transactions with operations required posting authority cannot be combined
             *  with transactions requiring active or owner authority. This is for ease of
             *  implementation. Future versions of authority verification may be able to
             *  check for the merged authority of active and posting.
             */
            if (required_posting.size()) {
                FC_ASSERT(required_active.size() == 0);
                FC_ASSERT(required_owner.size() == 0);
                FC_ASSERT(other.size() == 0);

                sign_state s(sigs, get_posting, avail);
                s.max_recursion = max_recursion_depth;

                for (const auto& id : posting_approvals) {
                   s.approved_by[id] = false;
                }

                for (const auto& id: required_posting) {
                    if (!s.check_authority(id) &&
                        !s.check_authority(get_active(id)) &&
                        !s.check_authority(get_owner(id))
                    ) {
                        missing_accounts.push_back(id);
                    }
                }

                GOLOS_CTOR_ASSERT(
                    missing_accounts.empty(),
                    tx_missing_posting_auth,
                    [&](auto& e) {
                        s.remove_unused_signatures();
                        e.used_signatures = std::move(s.used_signatures);
                        e.missing_accounts = std::move(missing_accounts);
                        e.append_log(
                            GOLOS_ASSERT_MESSAGE("Missing Posting Authority ${id}", ("id", e.missing_accounts)));
                    });

                assert_unused_approvals(s);
                return;
            }

            sign_state s(sigs, get_active, avail);
            s.max_recursion = max_recursion_depth;
            for (auto& id: active_aprovals) {
                s.approved_by[id] = false;
            }
            for (auto& id: owner_approvals) {
                s.approved_by[id] = false;
            }

            for (const auto& auth: other) {
                if (!s.check_authority(auth)) {
                    missing_auths.push_back(auth);
                }
            }

            GOLOS_CTOR_ASSERT(
                missing_auths.empty(),
                tx_missing_other_auth,
                [&](auto& e) {
                    e.missing_auths = std::move(missing_auths);
                    e.append_log(
                        GOLOS_ASSERT_MESSAGE("Missing Authority", ("auth", e.missing_auths)));
                });

            // fetch all of the top level authorities
            for (const auto& id: required_active) {
                if (!s.check_authority(id) && !s.check_authority(get_owner(id))) {
                    missing_accounts.push_back(id);
                }
            }

            GOLOS_CTOR_ASSERT(
                missing_accounts.empty(),
                tx_missing_active_auth,
                [&](auto& e) {
                    s.remove_unused_signatures();
                    e.used_signatures = std::move(s.used_signatures);
                    e.missing_accounts = std::move(missing_accounts);
                    e.append_log(
                        GOLOS_ASSERT_MESSAGE("Missing Active Authority ${id}", ("id", e.missing_accounts)));
                });

            for (const auto& id: required_owner) {
                if (owner_approvals.find(id) == owner_approvals.end() && !s.check_authority(get_owner(id))) {
                    missing_accounts.push_back(id);
                } else {
                    s.approved_by[id] = true;
                }
            }

            GOLOS_CTOR_ASSERT(
                missing_accounts.empty(),
                tx_missing_owner_auth,
                [&](auto& e) {
                    s.remove_unused_signatures();
                    e.used_signatures = std::move(s.used_signatures);
                    e.missing_accounts = std::move(missing_accounts);
                    e.append_log(
                        GOLOS_ASSERT_MESSAGE("Missing Owner Authority ${id}", ("id", e.missing_accounts)));
                });

            assert_unused_approvals(s);
        } FC_CAPTURE_AND_RETHROW((ops)(sigs)) }


        flat_set<public_key_type> signed_transaction::get_signature_keys(const chain_id_type &chain_id) const {
            try {
                auto d = sig_digest(chain_id);
                flat_set<public_key_type> result;
                for (const auto &sig : signatures) {
                    GOLOS_ASSERT(
                        result.insert(fc::ecc::public_key(sig, d)).second,
                        tx_duplicate_sig,
                        "Duplicate Signature detected");
                }
                return result;
            } FC_CAPTURE_AND_RETHROW()
        }


        set<public_key_type> signed_transaction::get_required_signatures(
                const chain_id_type &chain_id,
                const flat_set<public_key_type> &available_keys,
                const authority_getter &get_active,
                const authority_getter &get_owner,
                const authority_getter &get_posting,
                uint32_t max_recursion_depth) const {
            flat_set<account_name_type> required_active;
            flat_set<account_name_type> required_owner;
            flat_set<account_name_type> required_posting;
            vector<authority> other;
            get_required_authorities(required_active, required_owner, required_posting, other);

            /** posting authority cannot be mixed with active authority in same transaction */
            if (required_posting.size()) {
                sign_state s(get_signature_keys(chain_id), get_posting, available_keys);
                s.max_recursion = max_recursion_depth;

                FC_ASSERT(!required_owner.size());
                FC_ASSERT(!required_active.size());
                for (auto &posting : required_posting) {
                    s.check_authority(posting);
                }

                s.remove_unused_signatures();

                set<public_key_type> result;

                for (auto &provided_sig : s.provided_signatures) {
                    if (available_keys.find(provided_sig.first) !=
                        available_keys.end()) {
                        result.insert(provided_sig.first);
                    }
                }

                return result;
            }


            sign_state s(get_signature_keys(chain_id), get_active, available_keys);
            s.max_recursion = max_recursion_depth;

            for (const auto &auth : other) {
                s.check_authority(auth);
            }
            for (auto &owner : required_owner) {
                s.check_authority(get_owner(owner));
            }
            for (auto &active : required_active) {
                s.check_authority(active);
            }

            s.remove_unused_signatures();

            set<public_key_type> result;

            for (auto &provided_sig : s.provided_signatures) {
                if (available_keys.find(provided_sig.first) !=
                    available_keys.end()) {
                    result.insert(provided_sig.first);
                }
            }

            return result;
        }

        set<public_key_type> signed_transaction::minimize_required_signatures(
                const chain_id_type &chain_id,
                const flat_set<public_key_type> &available_keys,
                const authority_getter &get_active,
                const authority_getter &get_owner,
                const authority_getter &get_posting,
                uint32_t max_recursion
        ) const {
            set<public_key_type> s = get_required_signatures(chain_id, available_keys, get_active, get_owner, get_posting, max_recursion);
            flat_set<public_key_type> result(s.begin(), s.end());

            for (const public_key_type &k : s) {
                result.erase(k);
                try {
                    golos::protocol::verify_authority(operations, result, get_active, get_owner, get_posting, max_recursion);
                    continue;  // element stays erased if verify_authority is ok
                }
                catch (const tx_missing_owner_auth &e) {
                }
                catch (const tx_missing_active_auth &e) {
                }
                catch (const tx_missing_posting_auth &e) {
                }
                catch (const tx_missing_other_auth &e) {
                }
                result.insert(k);
            }
            return set<public_key_type>(result.begin(), result.end());
        }

        void signed_transaction::verify_authority(
                const chain_id_type &chain_id,
                const authority_getter &get_active,
                const authority_getter &get_owner,
                const authority_getter &get_posting,
                uint32_t max_recursion) const {
            try {
                golos::protocol::verify_authority(operations, get_signature_keys(chain_id), get_active, get_owner, get_posting, max_recursion);
            } FC_CAPTURE_AND_RETHROW((*this))
        }

    }
} // golos::protocol
