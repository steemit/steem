#pragma once
#include <steem/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <fc/crypto/restartable_sha256.hpp>

namespace steem { namespace plugins { namespace block_log_info {

using namespace std;
using namespace steem::chain;

#ifndef STEEM_BLOCK_LOG_INFO_SPACE_ID
#define STEEM_BLOCK_LOG_INFO_SPACE_ID 14
#endif

enum block_log_info_object_types
{
   block_log_hash_state_object_type      = ( STEEM_BLOCK_LOG_INFO_SPACE_ID << 8 )    ,
   block_log_pending_message_object_type = ( STEEM_BLOCK_LOG_INFO_SPACE_ID << 8 ) + 1,
};

class block_log_hash_state_object : public object< block_log_hash_state_object_type, block_log_hash_state_object >
{
   public:
      template< typename Constructor, typename Allocator >
      block_log_hash_state_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      block_log_hash_state_object() {}

      id_type                  id;

      uint64_t                 total_size = 0;
      fc::restartable_sha256   rsha256;
      uint64_t                 last_interval = 0;
};

struct block_log_message_data
{
   uint32_t block_num = 0;
   uint64_t total_size = 0;
   uint64_t current_interval = 0;
   fc::restartable_sha256 rsha256;
};

class block_log_pending_message_object : public object< block_log_pending_message_object_type, block_log_pending_message_object >
{
   public:
      template< typename Constructor, typename Allocator >
      block_log_pending_message_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      block_log_pending_message_object() {}

      id_type                  id;

      block_log_message_data   data;
};

typedef block_log_hash_state_object::id_type block_log_hash_state_id_type;
typedef block_log_pending_message_object::id_type block_log_pending_message_id_type;

typedef multi_index_container<
   block_log_hash_state_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< block_log_hash_state_object, block_log_hash_state_id_type, &block_log_hash_state_object::id > >
   >,
   allocator< block_log_hash_state_object >
> block_log_hash_state_index;

typedef multi_index_container<
   block_log_pending_message_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< block_log_pending_message_object, block_log_pending_message_id_type, &block_log_pending_message_object::id > >
   >,
   allocator< block_log_pending_message_object >
> block_log_pending_message_index;

} } } // steem::plugins::block_log_info


FC_REFLECT( steem::plugins::block_log_info::block_log_hash_state_object, (id)(total_size)(rsha256)(last_interval) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::block_log_info::block_log_hash_state_object, steem::plugins::block_log_info::block_log_hash_state_index )

FC_REFLECT( steem::plugins::block_log_info::block_log_message_data, (block_num)(total_size)(current_interval)(rsha256) )

FC_REFLECT( steem::plugins::block_log_info::block_log_pending_message_object, (id)(data) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::block_log_info::block_log_pending_message_object, steem::plugins::block_log_info::block_log_pending_message_index )
