#pragma once

#include <golos/plugins/chain/plugin.hpp>
#include <golos/chain/comment_object.hpp>
#include <golos/chain/account_object.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <appbase/application.hpp>
#include <golos/plugins/social_network/api_object/comment_api_object.hpp>
#include <golos/plugins/social_network/tag/social_network_sort.hpp>

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos { namespace plugins { namespace social_network { namespace tags {
    using namespace golos::chain;
    using namespace boost::multi_index;
    using chainbase::object;
    using chainbase::object_id;
    using chainbase::allocator;
    using golos::chain::account_object;

    //
    // Plugins should #define their SPACE_ID's so plugins with
    // conflicting SPACE_ID assignments can be compiled into the
    // same binary (by simply re-assigning some of the conflicting #defined
    // SPACE_ID's in a build script).
    //
    // Assignment of SPACE_ID's cannot be done at run-time because
    // various template automagic depends on them being known at compile
    // time.
    //
#ifndef TAG_SPACE_ID
#define TAG_SPACE_ID 5
#endif

    using tag_name_type = fc::fixed_string<fc::sha256> ;

    enum class tag_type: uint8_t {
        tag,
        language
    };

    // Plugins need to define object type IDs such that they do not conflict
    // globally. If each plugin uses the upper 8 bits as a space identifier,
    // with 0 being for chain, then the lower 8 bits are free for each plugin
    // to define as they see fit.
    enum tags_object_types {
        tag_object_type = (TAG_SPACE_ID << 8),
        tag_stats_object_type = (TAG_SPACE_ID << 8) + 1,
        author_tag_stats_object_type = (TAG_SPACE_ID << 8) + 2,
        language_object_type = (TAG_SPACE_ID << 8) + 3
    };

    /**
     *  The purpose of the tag object is to allow the generation and listing of
     *  all top level posts by a string tag.  The desired sort orders include:
     *
     *  1. created - time of creation
     *  2. maturing - about to receive a payout
     *  3. active - last reply the post or any child of the post
     *  4. netvotes - individual accounts voting for post minus accounts voting against it
     *
     *  When ever a comment is modified, all tag_objects for that comment are updated to match.
     */
    class tag_object: public object<tag_object_type, tag_object> {
    public:
        template<typename Constructor, typename Allocator>
        tag_object(Constructor&& c, allocator<Allocator> a) {
            c(*this);
        }

        id_type id;

        tag_name_type name;
        tag_type type = tag_type::tag;
        time_point_sec created;
        time_point_sec active;
        time_point_sec updated;
        time_point_sec cashout;
        int64_t net_rshares = 0;
        int32_t net_votes = 0;
        int32_t children = 0;
        double hot = 0;
        double trending = 0;

        share_type promoted_balance = 0;

        /**
         *  Used to track the total rshares^2 of all children, this is used for indexing purposes. A discussion
         *  that has a nested comment of high value should promote the entire discussion so that the comment can
         *  be reviewed.
         */
        fc::uint128_t children_rshares2;

        account_object::id_type author;
        comment_object::id_type parent;
        comment_object::id_type comment;

        bool is_post() const {
            return parent == comment_object::id_type();
        }
    };

    using tag_id_type = object_id<tag_object> ;


    struct by_author_comment;
    struct by_comment;
    struct by_tag;

    using tag_index = multi_index_container<
        tag_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<tag_object, tag_object::id_type, &tag_object::id>>,
            ordered_unique<
                tag<by_comment>,
                composite_key<
                    tag_object,
                    member<tag_object, comment_object::id_type, &tag_object::comment>,
                    member<tag_object, tag_object::id_type, &tag_object::id> >,
                composite_key_compare<
                    std::less<comment_object::id_type>,
                    std::less<tag_id_type>>>,
            ordered_unique<
                tag<by_author_comment>,
                composite_key<
                    tag_object,
                    member<tag_object, account_object::id_type, &tag_object::author>,
                    member<tag_object, comment_object::id_type, &tag_object::comment>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::less<account_object::id_type>,
                    std::less<comment_object::id_type>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<by_tag>,
                composite_key<
                    tag_object,
                    member<tag_object, tag_name_type, &tag_object::name>,
                    member<tag_object, tag_type, &tag_object::type>>,
                composite_key_compare<
                    std::less<tag_name_type>,
                    std::less<tag_type>>>,
            ordered_non_unique<
                tag<sort::by_created>,
                composite_key<
                    tag_object,
                    member<tag_object, time_point_sec, &tag_object::created>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<time_point_sec>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_active>,
                composite_key<
                    tag_object,
                    member<tag_object, time_point_sec, &tag_object::active>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<time_point_sec>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_updated>,
                composite_key<
                    tag_object,
                    member<tag_object, time_point_sec, &tag_object::updated>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<time_point_sec>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_promoted>,
                composite_key<
                    tag_object,
                    member<tag_object, share_type, &tag_object::promoted_balance>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<share_type>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_net_rshares>,
                composite_key<
                    tag_object,
                    member<tag_object, int64_t, &tag_object::net_rshares>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<int64_t>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_net_votes>,
                composite_key<
                    tag_object,
                    member<tag_object, int32_t, &tag_object::net_votes>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<int32_t>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_children>,
                composite_key<
                    tag_object,
                    member<tag_object, int32_t, &tag_object::children>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<int32_t>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_hot>,
                composite_key<
                    tag_object,
                    member<tag_object, double, &tag_object::hot>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<double>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_trending>,
                composite_key<
                    tag_object,
                    member<tag_object, double, &tag_object::trending>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::greater<double>,
                    std::less<tag_id_type>>>,
            ordered_non_unique<
                tag<sort::by_cashout>,
                composite_key<
                    tag_object,
                    member<tag_object, time_point_sec, &tag_object::cashout>,
                    member<tag_object, tag_id_type, &tag_object::id> >,
                composite_key_compare<
                    std::less<time_point_sec>,
                    std::less<tag_id_type>>>>,
        allocator<tag_object>>;

/**
     *  The purpose of this index is to quickly identify how popular various tags by maintaining various sums over
     *  all posts under a particular tag
     */
    class tag_stats_object: public object<tag_stats_object_type, tag_stats_object> {
    public:
        template<typename Constructor, typename Allocator>
        tag_stats_object(Constructor&& c, allocator<Allocator>) {
            c(*this);
        }

        tag_stats_object() {
        }

        id_type id;

        tag_name_type name;
        tag_type type;
        fc::uint128_t total_children_rshares2;
        asset total_payout = asset(0, SBD_SYMBOL);
        int32_t net_votes = 0;
        uint32_t top_posts = 0;
        uint32_t comments = 0;
    };

    using tag_stats_id_type = object_id<tag_stats_object>;

    struct by_trending;

    using tag_stats_index = multi_index_container<
        tag_stats_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<tag_stats_object, tag_stats_id_type, &tag_stats_object::id>>,
            ordered_unique<
                tag<by_tag>,
                composite_key<
                    tag_stats_object,
                    member<tag_stats_object, tag_type, &tag_stats_object::type>,
                    member<tag_stats_object, tag_name_type, &tag_stats_object::name>>,
                composite_key_compare<
                    std::less<tag_type>,
                    std::less<tag_name_type>>>,
            ordered_non_unique<
                tag<by_trending>,
                composite_key<
                    tag_stats_object,
                    member<tag_stats_object, tag_type, &tag_stats_object::type>,
                    member<tag_stats_object, fc::uint128_t, &tag_stats_object::total_children_rshares2>,
                    member<tag_stats_object, tag_name_type, &tag_stats_object::name>>,
                composite_key_compare<
                    std::less<tag_type>,
                    std::greater<uint128_t>,
                    std::less<tag_name_type>>>>,
        allocator<tag_stats_object> > ;

    /**
     *  This purpose of this object is to maintain stats about which tags an author uses, how frequnetly, and
     *  how many total earnings of all posts by author in tag.  It also allows us to answer the question of which
     *  authors earn the most in each tag category.  This helps users to discover the best bloggers to follow for
     *  particular tags.
     */
    class author_tag_stats_object: public object<author_tag_stats_object_type, author_tag_stats_object> {
    public:
        template<typename Constructor, typename Allocator>
        author_tag_stats_object(Constructor&& c, allocator<Allocator>) {
            c(*this);
        }

        id_type id;
        account_object::id_type author;
        tag_name_type name;
        tag_type type;
        asset total_rewards = asset(0, SBD_SYMBOL);
        uint32_t total_posts = 0;
    };

    using author_tag_stats_id_type = object_id<author_tag_stats_object>;

    struct by_author_tag_posts;
    struct by_author_posts_tag;

    using author_tag_stats_index = multi_index_container<
        author_tag_stats_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<author_tag_stats_object, author_tag_stats_id_type, &author_tag_stats_object::id> >,
            ordered_unique<
                tag<by_author_posts_tag>,
                composite_key<
                    author_tag_stats_object,
                    member<author_tag_stats_object, account_object::id_type, &author_tag_stats_object::author>,
                    member<author_tag_stats_object, tag_type, &author_tag_stats_object::type>,
                    member<author_tag_stats_object, uint32_t, &author_tag_stats_object::total_posts>,
                    member<author_tag_stats_object, tag_name_type, &author_tag_stats_object::name>>,
                composite_key_compare<
                    std::less<account_object::id_type>,
                    std::less<tag_type>,
                    std::greater<uint32_t>,
                    std::less<tag_name_type>>>,
            ordered_unique<
                tag<by_author_tag_posts>,
                composite_key<
                    author_tag_stats_object,
                    member<author_tag_stats_object, account_object::id_type, &author_tag_stats_object::author>,
                    member<author_tag_stats_object, tag_type, &author_tag_stats_object::type>,
                    member<author_tag_stats_object, tag_name_type, &author_tag_stats_object::name>,
                    member<author_tag_stats_object, uint32_t, &author_tag_stats_object::total_posts> >,
                composite_key_compare<
                    std::less<account_object::id_type>,
                    std::less<tag_type>,
                    std::less<tag_name_type>,
                    std::greater<uint32_t>>>>,
        allocator<author_tag_stats_object>>;

    /**
     * Cashed list of languages
     */
    class language_object: public object<language_object_type, language_object> {
    public:
        template<typename Constructor, typename Allocator>
        language_object(Constructor&& c, allocator<Allocator> a) {
            c(*this);
        }

        id_type id;
        tag_name_type name;
    };

    using language_id_type = object_id<language_object>;

    using language_index = multi_index_container<
        language_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<language_object, language_id_type, &language_object::id>>,
            ordered_unique<
                tag<by_tag>,
                member<language_object, tag_name_type, &language_object::name>>>,
        allocator<language_object>>;

    /**
     * Used to parse the metadata from the comment json_meta field.
     */
    struct comment_metadata {
        std::set<std::string> tags;
        std::string language;
    };

    comment_metadata get_metadata(const comment_api_object &c);

} } } } // golos::plugins::social_network::tags


CHAINBASE_SET_INDEX_TYPE(
    golos::plugins::social_network::tags::tag_object, golos::plugins::social_network::tags::tag_index)

CHAINBASE_SET_INDEX_TYPE(
    golos::plugins::social_network::tags::tag_stats_object, golos::plugins::social_network::tags::tag_stats_index)

CHAINBASE_SET_INDEX_TYPE(
    golos::plugins::social_network::tags::author_tag_stats_object,
    golos::plugins::social_network::tags::author_tag_stats_index)

CHAINBASE_SET_INDEX_TYPE(
    golos::plugins::social_network::tags::language_object,
    golos::plugins::social_network::tags::language_index)

FC_REFLECT((golos::plugins::social_network::tags::comment_metadata), (tags)(language))

