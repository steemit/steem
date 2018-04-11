#ifndef GOLOS_DISCUSSION_QUERY_H
#define GOLOS_DISCUSSION_QUERY_H

#include <fc/optional.hpp>
#include <fc/variant_object.hpp>
#include <fc/network/ip.hpp>

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <fc/exception/exception.hpp>

#ifndef DEFAULT_VOTE_LIMIT
#  define DEFAULT_VOTE_LIMIT 10000
#endif

#ifndef DEFAULT_QUERY_DEPTH
#  define DEFAULT_QUERY_DEPTH 100000
#endif

namespace golos { namespace plugins { namespace social_network {
    /**
     * @class discussion_query
     * @brief The discussion_query structure implements the RPC API param set.
     *  Defines the arguments to a query as a struct so it can be easily extended
     */

    class discussion_query {
    public:
        void validate() const;

        uint32_t                  start = 0; ///< the discussions from position
        uint32_t                  limit = 0; ///< the discussions return amount top limit
        uint32_t                  depth = DEFAULT_QUERY_DEPTH; ///< maximum depth of searching
        std::set<std::string>     select_tags; ///< list of tags to include, posts without these tags are filtered
        std::set<std::string>     filter_tags; ///< list of tags to exclude, posts with these tags are filtered;
        std::set<std::string>     select_languages; ///< list of language to select
        std::set<std::string>     filter_languages; ///< list of language to filter
        uint32_t                  truncate_body = 0; ///< the amount of bytes of the post body to return, 0 for all
        uint32_t                  vote_limit = DEFAULT_VOTE_LIMIT; ///< limit for active votes
        std::set<std::string>     select_authors; ///< list of authors to select
        fc::optional<std::string> start_author; ///< the author of discussion to start searching from
        fc::optional<std::string> start_permlink; ///< the permlink of discussion to start searching from
        fc::optional<std::string> parent_author; ///< the author of parent discussion
        fc::optional<std::string> parent_permlink; ///< the permlink of parent discussion
    };
} } } // golos::plugins::social_network

FC_REFLECT(
    (golos::plugins::social_network::discussion_query),
        (select_tags)(filter_tags)(select_authors)(truncate_body)(vote_limit)
        (start_author)(start_permlink)(parent_author)
        (parent_permlink)(start)(limit)(select_languages)(filter_languages));

#endif //GOLOS_DISCUSSION_QUERY_H
