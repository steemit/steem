#ifndef GOLOS_FEED_HISTORY_API_OBJ_HPP
#define GOLOS_FEED_HISTORY_API_OBJ_HPP

#include <golos/chain/steem_objects.hpp>

namespace golos {
    namespace plugins {
        namespace database_api {
            using golos::chain::feed_history_id_type;
            struct feed_history_api_object {
                feed_history_api_object(const golos::chain::feed_history_object &f) :
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

FC_REFLECT((golos::plugins::database_api::feed_history_api_object), (id)(current_median_history)(price_history))
#endif //GOLOS_FEED_HISTORY_API_OBJ_HPP
