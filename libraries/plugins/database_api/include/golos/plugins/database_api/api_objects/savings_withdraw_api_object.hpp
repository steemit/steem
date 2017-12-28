#ifndef GOLOS_SAVINGS_WITHDRAW_API_OBJ_HPP
#define GOLOS_SAVINGS_WITHDRAW_API_OBJ_HPP

#include <golos/chain/steem_objects.hpp>

namespace golos {
    namespace plugins {
        namespace database_api {


            struct savings_withdraw_api_object {
                savings_withdraw_api_object(const golos::chain::savings_withdraw_object &o) : id(o.id), from(o.from), to(o.to),
                        memo(to_string(o.memo)), request_id(o.request_id), amount(o.amount), complete(o.complete) {
                }

                savings_withdraw_api_object() {
                }

                savings_withdraw_object::id_type id;
                account_name_type from;
                account_name_type to;
                string memo;
                uint32_t request_id = 0;
                asset amount;
                time_point_sec complete;
            };
        }
    }
}

FC_REFLECT((golos::plugins::database_api::savings_withdraw_api_object),
           (id)(from)(to)(memo)(request_id)(amount)(complete))

#endif //GOLOS_SAVINGS_WITHDRAW_API_OBJ_HPP
