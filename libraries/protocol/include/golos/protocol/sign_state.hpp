#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/types.hpp>

namespace golos { namespace protocol {

    using authority_getter = std::function< authority (const account_name_type&) > ;

    struct sign_state {
        /** returns true if we have a signature for this key or can
         * produce a signature for this key, else returns false.
         */
        bool signed_by(const public_key_type& k);

        bool check_authority(const account_name_type& id);

        /**
         *  Checks to see if we have signatures of the active authorites of
         *  the accounts specified in authority or the keys specified.
         */
        bool check_authority(const authority& au, uint32_t depth = 0);

        bool remove_unused_signatures();

        bool filter_unused_approvals();

        sign_state(
            const flat_set<public_key_type>& sigs,
            const authority_getter& a,
            const flat_set<public_key_type>& keys);

        const authority_getter& get_active;
        const fc::flat_set<public_key_type>& available_keys;

        fc::flat_map<public_key_type, bool> provided_signatures;
        std::vector<public_key_type> unused_signatures;
        std::vector<public_key_type> used_signatures;
        fc::flat_map<account_name_type, bool> approved_by;
        std::vector<account_name_type> unused_approvals;
        uint32_t max_recursion = STEEMIT_MAX_SIG_CHECK_DEPTH;
    };

} } // golos::protocol
