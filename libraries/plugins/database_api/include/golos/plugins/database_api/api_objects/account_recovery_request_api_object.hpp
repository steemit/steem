#ifndef GOLOS_ACCOUNT_RECOVERY_REQUEST_API_OBJ_HPP
#define GOLOS_ACCOUNT_RECOVERY_REQUEST_API_OBJ_HPP

#include <golos/chain/account_object.hpp>

namespace golos {
    namespace plugins {
        namespace database_api {


            struct account_recovery_request_api_object {
                account_recovery_request_api_object(const golos::chain::account_recovery_request_object &o) : id(o.id),
                        account_to_recover(o.account_to_recover), new_owner_authority(authority(o.new_owner_authority)),
                        expires(o.expires) {
                }

                account_recovery_request_api_object() {
                }

                account_recovery_request_object::id_type id;
                account_name_type account_to_recover;
                authority new_owner_authority;
                time_point_sec expires;
            };
        }
    }
}


FC_REFLECT((golos::plugins::database_api::account_recovery_request_api_object),
           (id)(account_to_recover)(new_owner_authority)(expires))


#endif //GOLOS_ACCOUNT_RECOVERY_REQUEST_API_OBJ_HPP
