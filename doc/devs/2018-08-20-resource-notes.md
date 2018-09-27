
# Resource dynamics for witnesses

In HF20, we are introducing the concept of resources.  A *resource* is anything the blockchain
needs to limit in the medium-to-long term.  (Short-term limits are important too, but they
should be implemented in the code as per-block limits, e.g. the block size limit.)

The resources currently implemented in the code are:

- Subsidized accounts.  Users can occasionally create accounts for free.
- History bytes.  The amount of space stuff takes up in blocks.
- Market bytes.  A different kind of space, it exists for historical reasons.  It might go away at some point.
- State bytes.   The amount of space stuff takes up in chain state.
- Execution time.  The amount of time stuff takes to run.

# Resource pool

For each resource, the blockchain maintains a pool of that resource.  Every block, several forces affect the
number of resources in the pool:

- Usage:  Resources consumed by user transactions are subtracted from the pool.
- Budget:  A fixed number of resources is added to the pool each block.
- Decay:  A fixed percentage of resources is subtracted from the pool each block.

# Resource decay

Usage and budget are pretty straightforward and easy to think about.  Decay is a little less intuitive.

One way to think about decay is "resources expire."  Steem had relatively little activity over its
first X months, does that mean users should now be able to burst to 30X times the current limit for
the next day?  That sounds pretty wrong.

To avoid this kind of thing, it seems like we should implement some mechanic that causes unused
resources to "expire" if no one uses them for a long time.

We could write expiration logic to track the age of different lumps of resources.  But that would be
complicated and need a huge number of state objects.

A much simpler implementation is to just say "`0.01%` of this resource expires every block."  In that case
`0.01%` is the per-block decay factor.

In theoretical analysis of this kind of system, you may hear some terms thrown around:

- *Exponential decay* means any process where the value of something declines by a percentage every second (or whatever time unit).
- The *equilibrium pool level* is how many resources are in the pool when the budget and decay would balance (if usage was zero).
- The *half-life* is how long it would take the resource pool to fall to `1/2` (`50%`) its initial level (if usage and budget were zero).
- The *characteristic time* is how long it would take the resource pool to fall to `1/e` (`36.8%`) its initial level (if usage and budget were zero).

You can think of the half-life or characteristic time as roughly "how long" it takes the system to "reclassify" a change in activity level
from "short-term burst, nothing to see here, little/no response needed" to "medium/long-term trend has definitely changed, we need to
respond appropriately."  This is a slightly fuzzy concept, because the "detection" and "response" are both "soft" continuous behavior.

# Resource credits

Every user has a "manabar" called *resource credits* (RC).

- The maximum RC an account can have is equal to its Steem Power (technically, VESTS).
- It takes 5 days for RC to regenerate from 0% to 100%.  (So more SP means more RC's per hour.)
- Users automatically spend RC whenever they transact. RCs are charged based on various resources a transaction can consume. Resources are things like execution time, state size, transaction size. The sum of costs of all resources are charges to the user's RCs. The one RC pool pays for _all_ resources.
- A user who doesn't have enough RC is unable to transact.
- The amount of RC required per byte [1] is a decreasing function of the number of bytes in the pool (so-called "price curve").

You can look at this system like a market:

- Supply is how many resources (bytes / accounts) the blockchain "allows" users.
- Demand is how many RC's users who want to use resources have.

[1] In general, per resource.  So replace "bytes" with "account creations," "CPU cycles," or whatever.

# FAQ

- Q: What is the difference between resources and RC's?
- A: Here's a quick rundown:

- Resources say *the blockchain as a whole* can consume history bytes at this rate, state bytes at that rate, etc.
- RC's say *a particular user Alice* can consume this much.
- Each of the 6 or so resources are tracked differently.
- Each user has only one type of RC.
- The subsidized accounts resource is tracked in consensus.
- RC's are wholly non-consensus, they do not exist in consensus code.

- Q: How are resource limits different from max block size?
- A: Max block size is a short-term limit with a timescale of 1 block.  Resource limits are medium-to-long term limits, with timescales of dozens to millions of blocks.

- Q: What is a good decay value to use?
- A: "Good" is subjective.  There are daily variations (because more users are concentrated in certain timezones and awake during certain times
of the day), and weekly variations (people use Steem more/less on weekends/holidays).  The default decay rate (347321) corresponds to a half-life of
5 days, which should fully absorb daily variations and partially absorb weekly variations.  Since the RC pricing is determined based on the pool level,
the half-life is also roughly approximately the timescale at which RC prices become hard to predict -- there is some value in having a longer timescale
for varying of transaction costs.

- Q: What is a good budget value to use?
- A: "Good" is subjective.  But it should probably be about 2x the actual desired level of usage of the resource.

- Q: Why 2x?
- A: The RC plugin makes usage anywhere close to the actual budget value require a huge RC spending level.  How huge is "huge"?  Basically, if
100% of the Steem Power stake is constantly using all of its RC to do transactions that only require a single resource, the usage will be
equal to the budget.  The price curve of RC's is designed with a very rough empirical model which implies the "natural" level of RC usage will
result in about `50%` of the budget is actually used.  Like all user behavior, this model might not fit the data, in which case "2x" might be
wrong, and it should actually be "1.5x" or "3x".  But it will probably not be as small as "1x," and it will probably not be as large as "100x".

- Q: Why not have the budget be the desired usage level of the resource?
- A: If you say "users should use this much," how would you even enforce that?  The amount of activity that users do is nothing more, and nothing
less, than the sum of each individual user's decisions.  The budget creates market forces that influence these decisions:  You need X Steem Power
to transact at R rate (today, and tomorrow the numbers will probably be similar, but over time they might become very different).  The code sets
the numbers so that the budget is an upper bound on user activity.  How far below that upper bound will the actual user activity be?  That
depends very much on the sum of users' individual decisions about how many resources to consume and how much SP heavy consumers are willing
and able to buy from light consumers to increase their consumption.



How much users actually transact is something that is impossible to predict, but easy to measure and adapt.

# Setting parameters

Currently, the only witness configurable resource is subsidized accounts.  Two parameters are set:

- `account_subsidy_budget` is the per-block budget.
- A value of 10,000 (`STEEM_ACCOUNT_SUBSIDY_PRECISION`) represents a budget of one subsidized account per block.
- `account_subsidy_decay` is the per-block decay rate.  A value of `2^36` (`STEEM_RD_DECAY_DENOM_SHIFT`) represents 100% decay rate.

Here is a Python script to convert from half-life, measured in days, to an appropriately scaled per-block decay rate:

```
#!/usr/bin/env python3

import math

STEEM_BLOCKS_PER_DAY = 20*60*24
STEEM_RD_DECAY_DENOM_SHIFT = 36
STEEM_MAX_WITNESSES = 21
STEEM_MAX_VOTED_WITNESSES_HF17 = 20

f = lambda d : int(0.5 + (1 << STEEM_RD_DECAY_DENOM_SHIFT) * (-math.expm1(-math.log(2.0) / (STEEM_BLOCKS_PER_DAY * d * STEEM_MAX_VOTED_WITNESSES_HF17 / STEEM_MAX_WITNESSES))))
print("A 5-day half-life corresponds to a decay constant of", f(5))
```




# Setting parameter values

What should resource budgets be?

This is a tricky question, much like asking what should the maximum block size be.

The question basically boils down to:

- How much is the community willing to tolerate increases in full-node replay times and hardware requirements?
- How do you balance these concerns against allowing users to transact regularly, with little SP?
- How much SP do legitimate users and service providers delegate, relative to how much SP spammers have?

Generous resource budgets mean users can transact a lot, and don't need a lot of SP.  This is great for
legitimate users' user experience and growth.  But ultimately spam and IT concerns mean a line must be
drawn.

Where the line should be drawn depends on several things that are utterly unpredictable:  The
trajectory of technology, economics, and user behavior in the future.

# Consensus vs non-consensus

There are two types of resource limits:

- Non-consensus limits can be changed/ignored by a "rogue" witness running their own code.
- Consensus limits are enforced by the network, a "rogue" witness cannot change them.

It may sound like consensus limits are better, because they're enforced more strictly.  However, that strict
checking makes upgrades painful:

- Upgrading non-consensus limits is a witness-only upgrade.
- Upgrading consensus limits is a hardfork, all `steemd` nodes must upgrade.

The subsidized account resource limit is consensus.  Other resource limits are non-consensus.  Why were things
divided that way?  It has to do with the consequences of violating the limits:

- If a witness ignores the subsidized account limit, people will get new accounts for free, that normally cost STEEM to create.  This is a medium-sized economic problem.
- If a witness ignores the other resource limits, their blocks might take a little longer [1] to process or use more memory.  This is a tiny IT problem.

For resource limits, having witness-only upgrades outweighs the problem.  Witnesses who have been around a long time know that, in the ancient past, Steem's bandwidth
algorithm was consensus.  It was then changed to non-consensus when the current bandwidth algorithm was implemented.  The new resource-based limits are non-consensus
for the most part, just like the current bandwidth algorithm.

[1] Could they take a lot longer?  No, the rogue witness would be limited by block size.  Transactions have some variation in how much CPU / memory they use relative
to their size.  But operations that allow users to take a huge amount of CPU / memory for a tiny number of bytes are attack vectors.  As good blockchain architects,
we should never implement such operations in the Steem source code!  Even the worst-case CPU cycles / memory bytes consumed by an attacker spamming the most
"efficient" attack (in terms of CPU cycles / memory bytes consumed per byte of transaction size) should still be limited by the max block size.

# Setting consensus parameters

Currently, only the consensus parameter (subsidized account creation) can be set.  It is settable through  `witness_set_parameters_operation`. See [here](../witness_parameters.md) for more details on the operation.
