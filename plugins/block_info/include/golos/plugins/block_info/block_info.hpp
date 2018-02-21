#pragma once

#include <golos/chain/steem_object_types.hpp>

namespace golos {
namespace plugins {
namespace block_info {

struct block_info {
    golos::chain::block_id_type block_id;
    uint32_t block_size = 0;
    uint32_t average_block_size = 0;
    uint64_t aslot = 0;
    uint32_t last_irreversible_block_num = 0;
    uint32_t num_pow_witnesses = 0;
};

struct block_with_info {
    golos::chain::signed_block block;
    block_info info;
};

} } }


FC_REFLECT( (golos::plugins::block_info::block_info),
    (block_id)
    (block_size)
    (average_block_size)
    (aslot)
    (last_irreversible_block_num)
    (num_pow_witnesses)
)


FC_REFLECT( (golos::plugins::block_info::block_with_info),
    (block)
    (info)
)
