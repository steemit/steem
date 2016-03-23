/**
 * Copyright (c) Cryptonomex, Inc., All Rights Reserved
 * See LICENSE.md for full license.
 */

#include <steemit/app/comment_api.hpp>

namespace steemit{ namespace app {

optional< comment > comment_api::get_comment_data( const comment_id_type& id ) const
{
   FC_ASSERT( false, "comment_api::get_comment --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_global_comments_by_vote( uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_global_comments_by_vote --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_global_comments_by_pending_pay( uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_global_comments_by_pending_pay --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_global_comments_by_payout( uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_global_comments_by_payout --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api:: get_global_comments_by_date( const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_global_comments_by_date --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_vote( const comment_id_type& id, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_vote --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_pending_pay( const comment_id_type& id, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_pending_pay --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_payout( const comment_id_type& id, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_payout --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_date( const comment_id_type& id, const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_date --- NOT YET IMPLEMENTED" );
}

optional< comment_node > comment_api::get_comment_tree_by_vote( const comment_id_type& id, uint32_t depth ) const
{
   FC_ASSERT( false, "comment_api::get_comment_tree_by_vote --- NOT YET IMPLEMENTED" );
}

optional< comment_node > comment_api::get_comment_tree_by_pending_pay( const comment_id_type& id, uint32_t depth ) const
{
   FC_ASSERT( false, "comment_api::get_comments_tree_by_pending_pay --- NOT YET IMPLEMENTED" );
}

optional< comment_node > comment_api::get_comment_tree_by_payout( const comment_id_type& id, uint32_t depth ) const
{
   FC_ASSERT( false, "comment_api::get_comment_tree_by_payout --- NOT YET IMPLEMENTED" );
}

optional< comment_node > comment_api::get_comment_tree_by_date( const comment_id_type& id, const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t depth ) const
{
   FC_ASSERT( false, "comment_api::get_comment_tree_by_date --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_author_by_vote( const account_id_type& author, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_author_by_vote --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_author_by_pending_pay( const account_id_type& auhtor, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_author_by_pending_pay --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_author_by_payout( const account_id_type& author, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_author_by_payout --- NOT YET IMPLEMENTED" );
}

vector< comment_id_type > comment_api::get_comments_by_author_by_date( const account_id_type& author, const fc::time_point_sec& start, const fc::time_point_sec& stop, uint32_t limit ) const
{
   FC_ASSERT( false, "comment_api::get_comments_by_author_by_date --- NOT YET IMPLEMENTED" );
}

} } // steemit