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

#include <golos/chain/comment_object.hpp>
#include <golos/chain/account_object.hpp>

#include <golos/plugins/social_network/api_object/discussion.hpp>

#ifndef DEFAULT_VOTE_LIMIT
#  define DEFAULT_VOTE_LIMIT 10000
#endif

namespace golos { namespace plugins { namespace social_network {
    using golos::chain::account_object;
    using golos::chain::comment_object;
    /**
     * @class discussion_query
     * @brief The discussion_query structure implements the RPC API param set.
     *  Defines the arguments to a query as a struct so it can be easily extended
     */

    struct discussion_query {
        void prepare();
        void validate() const;

        uint32_t                          limit = 0; ///< the discussions return amount top limit
        std::set<std::string>             select_tags; ///< list of tags to include, posts without these tags are filtered
        std::set<std::string>             filter_tags; ///< list of tags to exclude, posts with these tags are filtered;
        std::set<std::string>             select_languages; ///< list of language to select
        std::set<std::string>             filter_languages; ///< list of language to filter
        uint32_t                          truncate_body = 0; ///< the amount of bytes of the post body to return, 0 for all
        uint32_t                          vote_limit = DEFAULT_VOTE_LIMIT; ///< limit for active votes
        std::set<std::string>             select_authors; ///< list of authors to select
        fc::optional<std::string>         start_author; ///< the author of discussion to start searching from
        fc::optional<std::string>         start_permlink; ///< the permlink of discussion to start searching from
        fc::optional<std::string>         parent_author; ///< the author of parent discussion
        fc::optional<std::string>         parent_permlink; ///< the permlink of parent discussion

        discussion                        start_comment;
        discussion                        parent_comment;
        std::set<account_object::id_type> select_author_ids;

        bool has_tags_selector() const {
            return !select_tags.empty();
        }

        bool has_tags_filter() const {
            return !filter_tags.empty();
        }

        bool has_language_selector() const {
            return !select_languages.empty();
        }

        bool has_language_filter() const {
            return !filter_languages.empty();
        }

        bool is_good_tags(const discussion& d) const;

        bool has_author_selector() const {
            return !select_author_ids.empty();
        }

        bool is_good_author(const account_object::id_type& id) const {
            return !has_author_selector() || select_author_ids.count(id);
        }

        bool is_good_author(const std::string& name) const {
            return !has_author_selector() || select_authors.count(name);
        }

        bool is_good_parent(const comment_object::id_type& id) const {
            return !!parent_author || id == parent_comment.id;
        }

        bool has_start_comment() const {
            return !!start_author;
        }

        bool is_good_start(const comment_object::id_type& id) const {
            return !has_start_comment() || id == start_comment.id;
        }

        void reset_start_comment() {
            start_author.reset();
        }
    };

} } } // golos::plugins::social_network

FC_REFLECT(
    (golos::plugins::social_network::discussion_query),
        (select_tags)(filter_tags)(select_authors)(truncate_body)(vote_limit)
        (start_author)(start_permlink)(parent_author)
        (parent_permlink)(limit)(select_languages)(filter_languages));

#endif //GOLOS_DISCUSSION_QUERY_H
