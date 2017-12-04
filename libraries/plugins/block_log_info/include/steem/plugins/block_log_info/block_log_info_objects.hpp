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
   block_log_hash_state_object_type = ( STEEM_BLOCK_LOG_INFO_SPACE_ID << 8 )
};

class block_log_hash_state_object : public object< block_log_hash_state_object_type, block_log_hash_state_object >
{
   public:
      template< typename Constructor, typename Allocator >
      block_log_hash_state_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                  id;

      uint64_t                 total_size = 0;
      fc::restartable_sha256   rsha256;
};

typedef block_log_hash_state_object::id_type block_log_hash_state_id_type;

using namespace boost::multi_index;

struct by_key;

typedef multi_index_container<
   block_log_hash_state_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< block_log_hash_state_object, block_log_hash_state_id_type, &block_log_hash_state_object::id > >
   >,
   allocator< block_log_hash_state_object >
> block_log_hash_state_index;

} } } // steem::plugins::block_log_info


FC_REFLECT( steem::plugins::block_log_info::block_log_hash_state_object, (id)(total_size)(rsha256) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::block_log_info::block_log_hash_state_object, steem::plugins::block_log_info::block_log_hash_state_index )
