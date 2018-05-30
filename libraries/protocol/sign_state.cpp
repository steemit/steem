
#include <golos/protocol/sign_state.hpp>

namespace golos { namespace protocol {

    bool sign_state::signed_by(const public_key_type& k) {
        auto itr = provided_signatures.find(k);
        if (itr == provided_signatures.end()) {
            auto pk = available_keys.find(k);
            if (pk != available_keys.end()) {
                return provided_signatures[k] = true;
            }
            return false;
        }
        itr->second = true;
        return true;
    }

    bool sign_state::check_authority(const account_name_type& id) {
        auto itr = approved_by.find(id);
        if (approved_by.end() != itr) {
            itr->second = true;
            return true;
        }
        return check_authority(get_active(id));
    }

    bool sign_state::check_authority(const authority& auth, uint32_t depth) {
        uint32_t total_weight = 0;
        for (const auto& k: auth.key_auths) {
            if (signed_by(k.first)) {
                total_weight += k.second;
                if (total_weight >= auth.weight_threshold) {
                    return true;
                }
            }
        }

        for (const auto& a: auth.account_auths) {
            if (approved_by.find(a.first) == approved_by.end()) {
                if (depth == max_recursion) {
                    continue;
                }
                if (check_authority(get_active(a.first), depth + 1)) {
                    approved_by[a.first] = true;
                    total_weight += a.second;
                    if (total_weight >= auth.weight_threshold) {
                        return true;
                    }
                }
            } else {
                total_weight += a.second;
                if (total_weight >= auth.weight_threshold) {
                    return true;
                }
            }
        }
        return total_weight >= auth.weight_threshold;
    }

    bool sign_state::remove_unused_signatures() {
        used_signatures.clear();
        used_signatures.reserve(provided_signatures.size());
        unused_signatures.clear();
        unused_signatures.reserve(provided_signatures.size());
        for (const auto& sig: provided_signatures) {
            if (!sig.second) {
                unused_signatures.push_back(sig.first);
            } else {
                used_signatures.push_back(sig.first);
            }
        }

        for (const auto& sig: unused_signatures) {
            provided_signatures.erase(sig);
        }

        return unused_signatures.size() != 0;
    }

    bool sign_state::filter_unused_approvals() {
        unused_approvals.clear();
        unused_approvals.reserve(approved_by.size());
        for (const auto& approval: approved_by) {
            if (!approval.second) {
                unused_approvals.push_back(approval.first);
            }
        }

        return unused_approvals.size() != 0;
    }

    sign_state::sign_state(
        const flat_set<public_key_type>& sigs,
        const authority_getter& a,
        const flat_set<public_key_type>& keys
    ) : get_active(a),
        available_keys(keys)
    {
        for (const auto& key: sigs) {
            provided_signatures[key] = false;
        }
        approved_by["temp"] = true;
    }

} } // golos::protocol
