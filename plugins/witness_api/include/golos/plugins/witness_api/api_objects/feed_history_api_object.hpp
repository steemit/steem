#pragma once

#include <golos/chain/steem_objects.hpp>

namespace golos { namespace plugins { namespace witness_api {

            using namespace golos::chain;
            using namespace golos::protocol;

            struct feed_history_api_object {
                feed_history_api_object(const feed_history_object &f) :
                        id(f.id),
                        current_median_history(f.current_median_history),
                        price_history(f.price_history.begin(), f.price_history.end()) {
                }

                feed_history_api_object() {
                }

                feed_history_id_type id;
                price current_median_history;
                deque<price> price_history;
            };
        }
    }
}

FC_REFLECT((golos::plugins::witness_api::feed_history_api_object), (id)(current_median_history)(price_history))
