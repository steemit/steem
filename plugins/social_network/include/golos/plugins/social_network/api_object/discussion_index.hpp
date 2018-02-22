#include <string>
#include <vector>
#include <fc/include/fc/reflect/reflect.hpp>
namespace golos {
    namespace plugins {
        namespace social_network {
            struct discussion_index {
                std::string category;    /// category by which everything is filtered
                std::vector<std::string> trending;    /// pending lifetime payout
                std::vector<std::string> trending30;  /// pending lifetime payout
                std::vector<std::string> created;     /// creation date
                std::vector<std::string> responses;   /// creation date
                std::vector<std::string> updated;     /// creation date
                std::vector<std::string> active;      /// last update or reply
                std::vector<std::string> votes;       /// last update or reply
                std::vector<std::string> cashout;     /// last update or reply
                std::vector<std::string> maturing;    /// about to be paid out
                std::vector<std::string> best;        /// total lifetime payout
                std::vector<std::string> hot;         /// total lifetime payout
                std::vector<std::string> promoted;    /// pending lifetime payout
            };

        }
    }
}

FC_REFLECT((golos::plugins::social_network::discussion_index), (category)(trending)(trending30)(updated)(created)(responses)(active)(votes)(maturing)(best)(hot)(promoted)(cashout))