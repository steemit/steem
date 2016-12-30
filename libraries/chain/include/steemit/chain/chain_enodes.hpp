#pragma once

#include <steemit/protocol/asset.hpp>
#include <steemit/protocol/types.hpp>

#include <steemit/chain/enode.hpp>

#include <fc/reflect/reflect.hpp>

namespace steemit { namespace chain {

using steemit::protocol::account_name_type;
using steemit::protocol::asset;
using steemit::protocol::block_id_type;
using steemit::protocol::transaction_id_type;

class block_enode : public enode
{
   public:
      block_id_type          block_id;
};

class transaction_enode : public enode
{
   public:
      transaction_id_type    txid;
      uint32_t               txnum = 0;
};

class operation_enode : public enode
{
   public:
      uint32_t               opnum = 0;
};

class create_vesting_enode : public enode
{
   public:
      account_name_type      to_account;
      asset                  steem;
      asset                  new_vesting;
};

} }

FC_REFLECT_DERIVED(
   steemit::chain::block_enode,
   (steemit::chain::enode),
   (block_id)
   )

FC_REFLECT_DERIVED(
   steemit::chain::transaction_enode,
   (steemit::chain::enode),
   (txid)
   (txnum)
   )

FC_REFLECT_DERIVED(
   steemit::chain::operation_enode,
   (steemit::chain::enode),
   (opnum)
   )

FC_REFLECT_DERIVED(
   steemit::chain::create_vesting_enode,
   (steemit::chain::enode),
   (steem)
   (new_vesting)
   )

