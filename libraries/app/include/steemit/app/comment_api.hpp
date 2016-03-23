/**
 * Copyright (c) Cryptonomex, Inc., All Rights Reserved
 * See LICENSE.md for full license.
 */
#pragma once

#include <steemit/app/api.hpp>

#include <steemit/chain/steem_objects.hpp>

#include <fc/time.hpp>
#include <fc/optional.hpp>

namespace steemit{ namespace app {

using namespace steemit::chain;
using namespace std;

struct comment
{
   string                  author;
   string                  title;
   string                  body;
   fc::time_point_sec      created;
   int64_t                 net_votes      = 0;
   int64_t                 total_votes    = 0;
   int64_t                 pending_payout = 0;
   int64_t                 total_payout   = 0;
};

struct comment_node
{
   comment_id_type         id;
   vector< comment_node >  children;
};

class comment_api
{
   public:
      comment_api( application& app ):_app( app ){}

      /**
      * Gets the comment by comment id.
      *
      * @param id The comment id.
      *
      * @return The comment.
      */
      optional< comment > get_comment_data( const comment_id_type& id ) const;

      /**
      * Get current global top comments, by vote.
      *
      * @param limit Maximum number of comments to return. Max is 100.
      *
      * @return A list of the top comments, by vote.
      */
      vector< comment_id_type > get_global_comments_by_vote( uint32_t limit = 100 ) const;

      /**
      * Get global comments with the highest pending payout.
      *
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of comments with the highest pending payout.
      */
      vector< comment_id_type > get_global_comments_by_pending_pay( uint32_t limit = 100 ) const;

      /**
      * Get current global top comments, by payout.
      *
      * @param limit Maximum number of comments to return. Max is 100.
      *
      * @return A list of the current top comments, by payout.
      */
      vector< comment_id_type > get_global_comments_by_payout( uint32_t limit = 100 ) const;

      /**
      * Get current global comments, by date.
      *
      * @param start Start date for filtering comments.
      * @param stop Stop date for filtering posts.
      * @param limit Maximum number of comments to return. Max is 100.
      *
      * @return A list of the current comments, by date.
      */
      vector< comment_id_type > get_global_comments_by_date( const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t limit = 100 ) const;

      /**
      * Get all top comments on a comment, by vote.
      *
      * @param id The ID of the parent comment.
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of the top comments on an object, by vote.
      */
      vector< comment_id_type > get_comments_by_vote( const comment_id_type& id, uint32_t limit = 100 ) const;

      /**
      * Get all top comments on a comment, by pending payout.
      *
      * @param id The ID of the parent comment.
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of the top comments on an object, by pending payout.
      */
      vector< comment_id_type > get_comments_by_pending_pay( const comment_id_type& id, uint32_t limit = 100 ) const;

      /**
      * Get all top comments on a comment, by payout.
      *
      * @param id The ID of the parent comment.
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of the top comments on an object, by payout.
      */
      vector< comment_id_type > get_comments_by_payout( const comment_id_type& id, uint32_t limit = 100 ) const;

      /**
      * Get all comments on a comment, by date.
      *
      * @param id The ID of the parent comment.
      * @param start Start date for filtering comments.
      * @param stop Stop date for filtering posts.
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of the comments on an object, by date.
      */
      vector< comment_id_type > get_comments_by_date( const comment_id_type& id, const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t limit = 100 ) const;

      /**
      * Get the comment tree of a comment, sorted by votes.
      *
      * @oaram id The ID of the parent comment.
      * @param depth The depth of tree. 0 will return the entire tree.
      *
      * @return A tree of comments on this comment, sorted by vote.
      */
      optional< comment_node > get_comment_tree_by_vote( const comment_id_type& id, uint32_t depth = 0 ) const;

      /**
      * Get the comment tree of a comment, sorted by pending payout.
      *
      * @oaram id The ID of the parent comment.
      * @param depth The depth of tree. 0 will return the entire tree.
      *
      * @return A tree of comments on this comment, sorted by pending payout.
      */
      optional< comment_node > get_comment_tree_by_pending_pay( const comment_id_type& id, uint32_t depth = 0 ) const;

      /**
      * Get the comment tree on a comment, sorted by payout.
      *
      * @oaram id The ID of the parent comment.
      * @param depth The depth of tree. 0 will return the entire tree.
      *
      * @return A tree of comments on this comment, sorted by payout.
      */
      optional< comment_node > get_comment_tree_by_payout( const comment_id_type& id, uint32_t depth = 0 ) const;

      /**
      * Get the comment tree on a comment, sorted by date. Because posts can be updated, it is possible
      * for a child to have an older date than the parent. If the parent's date is outside of the date
      * range, none of its children will be returned, even if they would be in the range.
      *
      * @oaram id The ID of the parent comment.
      * @param start Start date for filtering comments.
      * @param stop Stop date for filtering posts.
      * @param depth The depth of tree. 0 will return the entire tree.
      *
      * @return A tree of comments on this comment, sorted by date.
      */
      optional< comment_node > get_comment_tree_by_date( const comment_id_type& id, const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t depth = 0 ) const;

      /**
      * Get comments by an author, sorted by votes.
      *
      * @param author The id of the author
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of comments by author, sorted by vote.
      */
      vector< comment_id_type > get_comments_by_author_by_vote( const account_id_type& author, uint32_t limit = 100 ) const;

      /**
      * Get comments by an author, sorted by pending pay.
      *
      * @param author The id of the author
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of comments by author, sorted by pending pay.
      */
      vector< comment_id_type > get_comments_by_author_by_pending_pay( const account_id_type& auhtor, uint32_t limit = 100 ) const;

      /**
      * Get comments by an author, sorted by payout.
      *
      * @param author The id of the author
      * @param limit Maximum number of comments. Max is 100.
      *
      * @return A list of comments by author, sorted by payout.
      */
      vector< comment_id_type > get_comments_by_author_by_payout( const account_id_type& author, uint32_t limit = 100 ) const;

      /**
      * Get comments by an author, sorted by date.
      *
      * @param author The id of the author
      * @param limit Maximum number of comments. Max is 100.
      * @param start Start date for filtering comments.
      * @param stop Stop date for filtering posts.
      *
      * @return A list of comments by author, sorted by date.
      */
      vector< comment_id_type > get_comments_by_author_by_date( const account_id_type& author, const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t limit = 100 ) const;

   private:
      application& _app;
};

} } // steemit::app
/*
FC_REFLECT( graphene::app::post_breif,
   (author)(permlink))
FC_REFLECT( graphene::app::post_summary,
   (author)(permlink)(title)(summary)(tags))
FC_REFLECT( graphene::app::post,
   (author)(permlink)(title)(summary)(tags)(body))
FC_REFLECT( graphene::app::comment,
   (author)(body)(children))

FC_API( graphene::app::blog_api,
   (get_post_body)
   (get_post_by_permlink)
   (get_posts_by_tag)
   (get_posts_by_tags)
   (get_top_posts_by_category)
   (get_historic_posts_by_category)
   (get_top_posts_by_tag)
   (get_historic_top_posts_by_tag)
   (get_top_users)
   (get_historic_top_users)
   (get_global_top_comments)
   (get_historic_top_comments)
   (get_top_comments_on_object)
   (get_historic_top_comments_on_object)
   (get_comments_by_date)
   (get_comment_tree)
)*/
