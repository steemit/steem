#ifndef GOLOS_CATEGORY_API_OBJ_HPP
#define GOLOS_CATEGORY_API_OBJ_HPP

#include <golos/chain/comment_object.hpp>

namespace golos {
    namespace plugins {
        namespace social_network {
            using golos::protocol::asset;
            using golos::protocol::share_type;
            struct category_api_object {
                category_api_object(const golos::chain::category_object &c) :
                        id(c.id),
                        name(to_string(c.name)),
                        abs_rshares(c.abs_rshares),
                        total_payouts(c.total_payouts),
                        discussions(c.discussions),
                        last_update(c.last_update) {
                }

                category_api_object() {
                }

                category_object::id_type id;
                std::string name;
                share_type abs_rshares;
                asset total_payouts;
                uint32_t discussions;
                time_point_sec last_update;
            };
        }
    }
}

FC_REFLECT((golos::plugins::social_network::category_api_object),
           (id)(name)(abs_rshares)(total_payouts)(discussions)(last_update))

#endif //GOLOS_CATEGORY_API_OBJ_HPP
