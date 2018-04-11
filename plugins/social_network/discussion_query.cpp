

#include <golos/plugins/social_network/api_object/discussion_query.hpp>

namespace golos { namespace plugins { namespace social_network {

    void discussion_query::validate() const {
        FC_ASSERT(limit <= 100);
        FC_ASSERT(depth > limit);
        FC_ASSERT(start <= depth - limit);

        for (auto& itr : filter_tags) {
            FC_ASSERT(select_tags.find(itr) == select_tags.end());
        }

        for (auto& itr : filter_languages) {
            FC_ASSERT(select_languages.find(itr) == select_languages.end());
        }

        FC_ASSERT(start_author.valid() == start_permlink.valid());

        FC_ASSERT(parent_author.valid() == parent_permlink.valid());
    }
} } } // golos::plugins::social_network

