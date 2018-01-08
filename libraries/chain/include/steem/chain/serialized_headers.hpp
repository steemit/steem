#pragma once

#define SERIALIZED_HEADERS
#define OWNER_API
#define SERIALIZABLE_BOOST_CONTAINERS
#include <serialize3/h/client_code/serialize_macros.h>

#include <steem/chain/serialize_operators.hpp>

#include <steem/protocol/asset_symbol.hpp>
#include <steem/protocol/asset.hpp>
#include <steem/protocol/fixed_string.hpp>
#include <steem/protocol/version.hpp>
#include <steem/protocol/types.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <chainbase/chainbase.hpp>

#include <steem/chain/steem_objects.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/hardfork_property_object.hpp>
//#include <steem/chain/node_property_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/witness_objects.hpp>
#include <steem/chain/shared_authority.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/chain/smt_objects/account_balance_object.hpp>
