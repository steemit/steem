#pragma once
#include <fc/time.hpp>

namespace steemit { namespace chain {

  struct account_keys
  {
    authority                 owner_key;
    authority                 active_key;
    authority                 posting_key;
    public_key_type           memo_key;
  };

  struct account_balances
  {
    vector<asset>             assets;
  };

  struct snapshot_summary
  {
    asset                     balance;
    asset                     sbd_balance;
    asset                     total_vesting_shares;
    asset                     total_vesting_fund_steem;
    uint32_t                  accounts_count;
  };

  struct account_summary
  {
    uint32_t                  id;
    string                    name;
    account_keys              keys;
    share_type                posting_rewards;
    share_type                curation_rewards;
    account_balances          balances;
    string                    json_metadata;
    string                    proxy;
    uint32_t                  post_count;
    string                    recovery_account;
    share_type                reputation;
  };

  struct snapshot_state
  {
    fc::time_point_sec        timestamp;
    uint32_t                  head_block_num;
    block_id_type             head_block_id;
    chain_id_type             chain_id;
    snapshot_summary          summary;

    vector<account_summary>   accounts;
  };

} }

FC_REFLECT( steemit::chain::account_keys, (owner_key)(active_key)(posting_key)(memo_key) )
FC_REFLECT( steemit::chain::account_balances, (assets) )
FC_REFLECT( steemit::chain::snapshot_summary, (balance)(sbd_balance)(total_vesting_shares)(total_vesting_fund_steem)(accounts_count) )
FC_REFLECT( steemit::chain::account_summary, (id)(name)(posting_rewards)(curation_rewards)(keys)(balances)(json_metadata)(proxy)(post_count)(recovery_account)(reputation) )
FC_REFLECT( steemit::chain::snapshot_state, (timestamp)(head_block_num)(head_block_id)(chain_id)(summary)(accounts) )
