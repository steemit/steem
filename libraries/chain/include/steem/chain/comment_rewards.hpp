#pragma once

#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/database.hpp>

namespace steem { namespace chain {

void process_comment_rewards( const database& db );

} } // steem::chain
