#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

//#include <golos/db2/database.hpp>
#include <chainbase/chainbase.hpp>

#include <golos/protocol/types.hpp>
#include <golos/protocol/authority.hpp>


namespace golos {
    namespace chain {

        using namespace boost::multi_index;

        using boost::multi_index_container;

        using chainbase::object;
        using chainbase::object_id;
        using chainbase::allocator;

        using golos::protocol::block_id_type;
        using golos::protocol::transaction_id_type;
        using golos::protocol::chain_id_type;
        using golos::protocol::account_name_type;
        using golos::protocol::share_type;

        typedef boost::interprocess::basic_string<char, std::char_traits<char>, allocator<char>> shared_string;

        inline std::string to_string(const shared_string &str) {
            return std::string(str.begin(), str.end());
        }

        inline void from_string(shared_string &out, const string &in) {
            out.assign(in.begin(), in.end());
        }

        typedef boost::interprocess::vector<char, allocator<char>> buffer_type;

        struct by_id;

        enum object_type {
            dynamic_global_property_object_type,
            account_object_type,
            account_authority_object_type,
            account_bandwidth_object_type,
            witness_object_type,
            transaction_object_type,
            block_summary_object_type,
            witness_schedule_object_type,
            comment_object_type,
            comment_vote_object_type,
            witness_vote_object_type,
            limit_order_object_type,
            feed_history_object_type,
            convert_request_object_type,
            liquidity_reward_balance_object_type,
            // old_operation_object_type,
            // old_account_history_object_type,
            // old_category_object_type,
            hardfork_property_object_type,
            withdraw_vesting_route_object_type,
            owner_authority_history_object_type,
            account_recovery_request_object_type,
            change_recovery_account_request_object_type,
            escrow_object_type,
            savings_withdraw_object_type,
            decline_voting_rights_request_object_type,
            block_stats_object_type,
            vesting_delegation_object_type,
            vesting_delegation_expiration_object_type
        };

        class dynamic_global_property_object;
        class account_object;
        class account_authority_object;
        class account_bandwidth_object;
        class witness_object;
        class transaction_object;
        class block_summary_object;
        class witness_schedule_object;
        class comment_object;
        class comment_vote_object;
        class witness_vote_object;
        class limit_order_object;
        class feed_history_object;
        class convert_request_object;
        class liquidity_reward_balance_object;
        class category_object;
        class hardfork_property_object;
        class withdraw_vesting_route_object;
        class owner_authority_history_object;
        class account_recovery_request_object;
        class change_recovery_account_request_object;
        class escrow_object;
        class savings_withdraw_object;
        class decline_voting_rights_request_object;
        class block_stats_object;
        class vesting_delegation_object;
        class vesting_delegation_expiration_object;

        typedef object_id<dynamic_global_property_object> dynamic_global_property_id_type;
        typedef object_id<account_object> account_id_type;
        typedef object_id<account_authority_object> account_authority_id_type;
        typedef object_id<account_bandwidth_object> account_bandwidth_id_type;
        typedef object_id<witness_object> witness_id_type;
        typedef object_id<transaction_object> transaction_object_id_type;
        typedef object_id<block_summary_object> block_summary_id_type;
        typedef object_id<witness_schedule_object> witness_schedule_id_type;
        typedef object_id<comment_object> comment_id_type;
        typedef object_id<comment_vote_object> comment_vote_id_type;
        typedef object_id<witness_vote_object> witness_vote_id_type;
        typedef object_id<limit_order_object> limit_order_id_type;
        typedef object_id<feed_history_object> feed_history_id_type;
        typedef object_id<convert_request_object> convert_request_id_type;
        typedef object_id<liquidity_reward_balance_object> liquidity_reward_balance_id_type;
        typedef object_id<category_object> category_id_type;
        typedef object_id<hardfork_property_object> hardfork_property_id_type;
        typedef object_id<withdraw_vesting_route_object> withdraw_vesting_route_id_type;
        typedef object_id<owner_authority_history_object> owner_authority_history_id_type;
        typedef object_id<account_recovery_request_object> account_recovery_request_id_type;
        typedef object_id<change_recovery_account_request_object> change_recovery_account_request_id_type;
        typedef object_id<escrow_object> escrow_id_type;
        typedef object_id<savings_withdraw_object> savings_withdraw_id_type;
        typedef object_id<decline_voting_rights_request_object> decline_voting_rights_request_id_type;
        typedef object_id<block_stats_object> block_stats_id_type;
        typedef object_id<vesting_delegation_object> vesting_delegation_id_type;
        typedef object_id<vesting_delegation_expiration_object> vesting_delegation_expiration_id_type;

        enum bandwidth_type {
            post,    ///< Rate limiting posting reward eligibility over time
            forum,   ///< Rate limiting for all forum related actins
            market,  ///< Rate limiting for all other actions
            old_forum,   ///< Rate limiting for all forum related actions (deprecated)
            old_market   ///< Rate limiting for all other actions (deprecated)
        };

    }
} //golos::chain

namespace fc {
    class variant;

    inline void to_variant(const golos::chain::shared_string &s, variant &var) {
        var = std::string(golos::chain::to_string(s));
    }

    inline void from_variant(const variant &var, golos::chain::shared_string &s) {
        auto str = var.as_string();
        s.assign(str.begin(), str.end());
    }

    template<typename T>
    void to_variant(const chainbase::object_id<T> &var, variant &vo) {
        vo = var._id;
    }

    template<typename T>
    void from_variant(const variant &vo, chainbase::object_id<T> &var) {
        var._id = vo.as_int64();
    }

    namespace raw {
        template<typename Stream, typename T>
        inline void pack(Stream &s, const chainbase::object_id<T> &id) {
            s.write((const char *)&id._id, sizeof(id._id));
        }

        template<typename Stream, typename T>
        inline void unpack(Stream &s, chainbase::object_id<T> &id) {
            s.read((char *)&id._id, sizeof(id._id));
        }
    }

    namespace raw {
        using chainbase::allocator;

        template<typename T>
        inline void pack(golos::chain::buffer_type &raw, const T &v) {
            auto size = pack_size(v);
            raw.resize(size);
            datastream<char *> ds(raw.data(), size);
            pack(ds, v);
        }

        template<typename T>
        inline void unpack(const golos::chain::buffer_type &raw, T &v) {
            datastream<const char *> ds(raw.data(), raw.size());
            unpack(ds, v);
        }

        template<typename T>
        inline T unpack(const golos::chain::buffer_type &raw) {
            T v;
            datastream<const char *> ds(raw.data(), raw.size());
            unpack(ds, v);
            return v;
        }
    }
}

namespace fc {

}

FC_REFLECT_ENUM(golos::chain::object_type,
        (dynamic_global_property_object_type)
                (account_object_type)
                (account_authority_object_type)
                (account_bandwidth_object_type)
                (witness_object_type)
                (transaction_object_type)
                (block_summary_object_type)
                (witness_schedule_object_type)
                (comment_object_type)
                (comment_vote_object_type)
                (witness_vote_object_type)
                (limit_order_object_type)
                (feed_history_object_type)
                (convert_request_object_type)
                (liquidity_reward_balance_object_type)
                // (old_operation_object_type)
                // (old_account_history_object_type)
                (hardfork_property_object_type)
                (withdraw_vesting_route_object_type)
                (owner_authority_history_object_type)
                (account_recovery_request_object_type)
                (change_recovery_account_request_object_type)
                (escrow_object_type)
                (savings_withdraw_object_type)
                (decline_voting_rights_request_object_type)
                (block_stats_object_type)
                (vesting_delegation_object_type)
                (vesting_delegation_expiration_object_type)
)

FC_REFLECT_TYPENAME((golos::chain::shared_string))
FC_REFLECT_TYPENAME((golos::chain::buffer_type))

FC_REFLECT_ENUM(golos::chain::bandwidth_type, (post)(forum)(market)(old_forum)(old_market))
