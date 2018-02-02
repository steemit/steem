#pragma once

#include <chainbase/chainbase.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/chain/steem_object_types.hpp>
#include <golos/protocol/types.hpp>
#include <golos/protocol/asset.hpp>
#include <golos/protocol/steem_virtual_operations.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef MARKET_HISTORY_SPACE_ID
#define MARKET_HISTORY_SPACE_ID 7
#endif

namespace golos {
    namespace plugins {
        namespace market_history {

            using namespace golos::protocol;
            using namespace boost::multi_index;
            using namespace chainbase;

            enum market_history_object_types {
                bucket_object_type = (MARKET_HISTORY_SPACE_ID << 8),
                order_history_object_type = (MARKET_HISTORY_SPACE_ID << 8) + 1
            };

            // Api params
            struct market_ticker {
                double latest = 0;
                double lowest_ask = 0;
                double highest_bid = 0;
                double percent_change = 0;
                asset steem_volume = asset(0, STEEM_SYMBOL);
                asset sbd_volume = asset(0, SBD_SYMBOL);
            };

            struct market_volume {
                asset steem_volume = asset(0, STEEM_SYMBOL);
                asset sbd_volume = asset(0, SBD_SYMBOL);
            };

            struct order {
                double price;
                share_type steem;
                share_type sbd;
            };

            struct order_book {
                vector <order> bids;
                vector <order> asks;
            };

            struct market_trade {
                time_point_sec date;
                asset current_pays;
                asset open_pays;
            };

            struct bucket_object
                    : public object<bucket_object_type, bucket_object> {
                template<typename Constructor, typename Allocator>
                bucket_object(Constructor &&c, allocator <Allocator> a) {
                    c(*this);
                }

                bucket_object() {
                }

                id_type id;

                fc::time_point_sec open;
                uint32_t seconds = 0;
                share_type high_steem;
                share_type high_sbd;
                share_type low_steem;
                share_type low_sbd;
                share_type open_steem;
                share_type open_sbd;
                share_type close_steem;
                share_type close_sbd;
                share_type steem_volume;
                share_type sbd_volume;

                golos::protocol::price high() const {
                    return asset(high_sbd, SBD_SYMBOL) /
                           asset(high_steem, STEEM_SYMBOL);
                }

                golos::protocol::price low() const {
                    return asset(low_sbd, SBD_SYMBOL) /
                           asset(low_steem, STEEM_SYMBOL);
                }
            };

            typedef object_id <bucket_object> bucket_id_type;


            struct order_history_object
                    : public object<order_history_object_type, order_history_object> {
                template<typename Constructor, typename Allocator>
                order_history_object(Constructor &&c, allocator <Allocator> a) {
                    c(*this);
                }

                id_type id;

                fc::time_point_sec time;
                golos::protocol::fill_order_operation op;
            };

            typedef object_id <order_history_object> order_history_id_type;

            struct by_id;
            struct by_bucket;
            typedef multi_index_container <
            bucket_object,
            indexed_by<
                    ordered_unique < tag < by_id>, member<bucket_object, bucket_id_type, &bucket_object::id>>,
            ordered_unique <tag<by_bucket>,
            composite_key<bucket_object,
                    member < bucket_object, uint32_t, &bucket_object::seconds>,
            member<bucket_object, fc::time_point_sec, &bucket_object::open>
            >,
            composite_key_compare <std::less<uint32_t>, std::less<fc::time_point_sec>>
            >
            >,
            allocator <bucket_object>
            >
            bucket_index;

            struct by_time;
            typedef multi_index_container <
            order_history_object,
            indexed_by<
                    ordered_unique < tag <
                    by_id>, member<order_history_object, order_history_id_type, &order_history_object::id>>,
            ordered_non_unique <tag<by_time>, member<order_history_object, time_point_sec, &order_history_object::time>>
            >,
            allocator <order_history_object>
            >
            order_history_index;
        }
    }
} // golos::plugins::market_history

FC_REFLECT((golos::plugins::market_history::market_ticker),
           (latest)(lowest_ask)(highest_bid)(percent_change)(steem_volume)(sbd_volume));
FC_REFLECT((golos::plugins::market_history::market_volume),
           (steem_volume)(sbd_volume));
FC_REFLECT((golos::plugins::market_history::order),
           (price)(steem)(sbd));
FC_REFLECT((golos::plugins::market_history::order_book),
           (bids)(asks));
FC_REFLECT((golos::plugins::market_history::market_trade),
           (date)(current_pays)(open_pays));


FC_REFLECT((golos::plugins::market_history::bucket_object),
           (id)
                   (open)(seconds)
                   (high_steem)(high_sbd)
                   (low_steem)(low_sbd)
                   (open_steem)(open_sbd)
                   (close_steem)(close_sbd)
                   (steem_volume)(sbd_volume))
CHAINBASE_SET_INDEX_TYPE(golos::plugins::market_history::bucket_object, golos::plugins::market_history::bucket_index)

FC_REFLECT((golos::plugins::market_history::order_history_object),(id)(time)(op))
CHAINBASE_SET_INDEX_TYPE(golos::plugins::market_history::order_history_object, golos::plugins::market_history::order_history_index)
