#include <fc/reflect/reflect.hpp>

namespace golos {
    namespace plugins {
        namespace social_network {

            struct category_index {
                std::vector<std::string> active;   /// recent activity
                std::vector<std::string> recent;   /// recently created
                std::vector<std::string> best;     /// total lifetime payout
            };

        }
    }
}

FC_REFLECT((golos::plugins::social_network::category_index), (active)(recent)(best))