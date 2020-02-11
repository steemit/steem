## Delegation Pools

Delegation pools are a solution primarily to the "push" approach of SP delegations. In order to "fund" new accounts, a surplus of SP must be delegated. This incentivizes abuse of faucets and ties up significant amounts of SP in delegations to accounts that may not be using it. This feature has a secondary benefit that allow users to pool RCs to fund actions for SMTs. SMTs are a unique entity in Steem in that actions are performed on behalf of an SMT (similarly to the implicit "blockchain" entity used for comment rewards, inflation, etc.) yet are still user created and thus need to be rate limited by some mechanism or else the blockchain would be open to DoS exploits/attacks.

The basic structure looks like this:

```
-------------                        -------------
| Account A |  \                  /  | Account D |
-------------   \ (in del)       /   -------------
                 \              /
-------------     \  ========  /     -------------
| Account B |  ----  | Pool |  ----  | Account E |
-------------     /  ========  \     -------------
                 /              \
-------------   /      (out del) \   -------------
| Account C |  /                  \  | Account F |
-------------                        -------------
```

Furthermore, each account can receive from multiple pools.

```
==========
| Pool A |  \
==========   \ (out del)
              \
==========     \  -----------
| Pool B |  ----  | Account |
==========     /  -----------
              /
==========   /
| Pool C |  /
==========
```

Unlike SP Delegations, the number of which are limitless so long as the account has sufficient SP, oputgoing RC delegations are limited. This is a conseqeuence of the interactions between soft and hard consensus.

For example, if Alice has 100 SP and delegates 75 RC to Pool A, then Alice still votes with 100 SP, but only has 25 RC to fund her actions. If Alice then delegates 50 SP to Bob, she only votes with 50 SP, but has delegated a total of 125 RC! That is a problem. Currently, RCs respect SP delegations. We could remove that behavior, fixing this problem, but breaking existing use cases for delegations.

It is worth noting that existing delegations could be replaced with a pool of the following form:

```
---------       ==================      -------
| Alice |  ---- | Anonymous Pool | ---- | Bob |
---------       ==================      -------
```

Instead, we need to create a delegation priority. SP Delegations will take priority over RC Delegations to preserve existing behavior. Continuing with the previous example, when Alice delegates 50 SP to Bob, her delegation to Pool A must implicitly decrease. The in delegation must have both fields for desired RCs and actual RCs delegated and the actual RCs delegated must decrease to 50 so that Alice isn't over subscribing her RCs. If Alice then realizes her mistake, she can chose to either decrease her RC delegation to Pool A or decrease her SP delegation to Bob. Either would fix the problem.

If she decides to decrease her SP delegation to Bob, then we need to adjust the actual RCs for her RC delegation to Pool A back up. For this simple case, this does not look bad, but if Alice delegates RCs to many pools, then these calculations can become quite expensive. RCs at their core are a system to prevent users from spamming costly actions on the blockchain. If the system which tracks this (RCs) is itself costly, then the system is not actually protecting the blockchain.

This is further exacerbated by calculations to determine how RCs are being removed from the delegations. An easy solution is to cap the number of delegations to a reasonable amount. This is currently capped at 40 delegations and should be sufficient for nearly every use case. This can be changed easily in the future due to the RC plugin being soft consensus.

Likewise, each account can only have a limited number of in delegations. When an account charges RCs, it needs to know which pools to charge to. An account can have three in delegations. To prevent an unknown pool from occupying a slot that a user does not wish to be occupied, those delegations are broken in to slots that the user white lists.

> - By default, the allowed delegator of Slot 0 is the creator.
> - By default, the allowed delegator of Slot 1 is the account recovery partner.
> - By default, the allowed delegator of Slot 2 is either the creator or the account recovery partner.
> - The (end-)user, or the user the slot is currently accessible to, can execute a (custom) operation to override the default and change that outdel slot's allowed delegator (ejecting any delegation currently using that outdel slot).

Each pool is be managed using a simple oversubstription model. The sum of out dels should be allowed to surpass the actual RCs of the pool as much as the pool owner desires. Each account receiving from the pool would not be able to use more than the amount specified by the pool owner, regardless of how much reserve RCs the pool contains and if the pool runs out of RCs altogether, then regardless of how many RCs a user has been delegated, they will not be able to use those RCs. It is the responsibility of the pool owner to manage the oversubscription and usage ratios to provide the desired level of serive to delegations.

```
(10 RC)                              (10 RC)
-------------                        -------------
| Account A |  \                  /  | Account D |
-------------   \                /   -------------
(5 RC)           \   (20 RC)    /    (10 RC)
-------------     \  ========  /     -------------
| Account B |  ----  | Pool |  ----  | Account E |
-------------     /  ========  \     -------------
(5 RC)           /              \    (10 RC)
-------------   /                \   -------------
| Account C |  /                  \  | Account F |
-------------                        -------------
```

The above example shows how accounts A, B, and C can pool 20 RC together to provide service worth 30 RCs. So long as account's D, E, and F don't use more than their 10 RC individually and never use more than 20 RC combined, none of the accounts will see any interupption in their ability to transact.


## SMT Delegation

Each SMT has a special case pool. To delegate to an SMT, users delegate to a pool whose name is the NAI string of the SMT. That pool is special in that there an single implicit out del that receives 100% of the pool. The model would looks like the following:

```
(10 RC)
-------------
| Account A |  \
-------------   \
(5 RC)           \   (20 RC)         (20 RC)
-------------     \  ========        -------------
| Account B |  ----  | Pool |  ----  |    SMT    |
-------------     /  ========        -------------
(5 RC)           /
-------------   /
| Account C |  /
-------------
```

Delegations to SMTs are used to power SMT specific actions, such as token emissions. Without such delegations, these actions will not be included on chain and it is likely that the SMT will be considered "dead". The number of RCs required to support such behavior will likely be relatively low, but SMT creators and communities should be consious of this requirement and delegation their RCs to the SMTs that they regularly use.

## Managing RC Delegation Pools

Delegation Pools are implemented as a second layer solution, also called "soft consensus". The rc_plugin has defined several of its own operations that can be included in a `custom_json_operation`.

### Delegate To Pool

Delegate SP to a pool.

The delegated SP no longer generates RC for from_account, instead it generates RC for to_pool.

Each account is limited to delegating to 40 pools.

```
struct delegate_to_pool_operation
{
   account_name_type     from_account;
   account_name_type     to_pool;
   asset                 amount;

   extensions_type       extensions;
};
```

### Delegate From Pool

Delegate SP from a pool to a user.

What is actually delegated are DRC (delegated RC). The from_pool account is allowed to "freely print" DRC from its own pool.  This is equal to the RC that pool user is allowed to consume. Pools may be over-subscribed (i.e. total DRC output is greater than RC input).

Executing this operation effectively _replaces_ the effect of the previous `delegate_drc_from_pool_operation` with the same `(to_account, to_slot)` pair.

When deciding whether the affected manabar sections are full or empty, the decision is always made in favor of `to_account`:

- When increasing delegation, new DRC is always created to fill the new manabar section.
- When removing delegation, DRC may be destroyed, but only to the extent necessary for the account not to exceed its newly reduced maximum.

```
struct delegate_drc_from_pool_operation
{
   account_name_type      from_pool;
   account_name_type      to_account;
   int8_t                 to_slot = 0;
   asset_symbol_type      asset_symbol;
   int64_t                drc_max_mana = 0;

   extensions_type        extensions;
};
```

### Set Slot Delegator

The purpose of `set_slot_delegator_operation` is to set the delegator allowed to delegate to a `(to_account, to_slot)` pair.

To delegate to a slot, from_pool must be considered the delegator of the slot. Normally the delegator is the account creator. The `set_slot_delegator_operation` allows a new delegator to be appointed by either the existing delegator, the account itself, or (in the case of slot 1) the slot may also be set by the account's recovery partner.

```
struct set_slot_delegator_operation
{
   account_name_type      from_pool;
   account_name_type      to_account;
   int8_t                 to_slot = 0;
   account_name_type      signer;

   extensions_type        extensions;
};
```