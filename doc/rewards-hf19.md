# Introduction

In this document we show the calculations used in the reward system for authors and curators in Hard Fork 0.19.

# Voting power

The voting power is a mechanism to have an stable economy. It is a mechanism to reduce the possibility that everyone could vote all the time and therefore a way to incentive the correct voting, proof-of-brain.

The voting power consists of a percentage, every time you vote the voting power is reduced a little bit, and at the same time it is regenerated at a rate of 20% per day (that is 100% in 5 days). This regeneration is not calculated every time a new block is created in the blockchain, but when you vote it does the calculation from the time since the last vote:

![1](http://quicklatex.com/cache3/70/ql_2f3783861d5762ba7b2d35eac399f270_l3.png)

where ```Bt``` is the block time, ```Vt``` is the last vote time of curator, ```t``` is the elapsed seconds, and ```rp``` is the regenerated power. Then it is added to the last voting power ```Vp0``` and limited to 100% in order to calculate the current voting power ```Vp```:

![2](http://quicklatex.com/cache3/5f/ql_3e02b4247f80e98f01e645eb3ca6fd5f_l3.png)

The curator can set a weight ```w``` for his vote, from 0% to 100%. This weight can be positive (upvote), or negative (downvote or flag), and the power used for both cases is the same, then the weighted power ```wp``` is:

![3](http://quicklatex.com/cache3/29/ql_1eb2275bede446cf86f4ad1d35520029_l3.png)

Finally, the voting power used ```pu``` is a small part of this value, and it is calculated using the following formula:

![4](http://quicklatex.com/cache3/db/ql_1078a274200a89aafa47cc9bc03fffdb_l3.png)

For example, if the voting power is 100% and the weight is 100%, then voting power used is ```(100% + 0.49%)/50=2.0098%``` which is rounded to 2%. If the voting power is 70% and the weight is 100%, then power used is ```(70% + 0.49%)/50=1.4098%``` which is rounded to 1.4%.

If we assume that 0.49% can be neglected in the formula. Then the power used can be simplified:

![5](http://quicklatex.com/cache3/29/ql_5e24e0bf048dd395085dcfd866909529_l3.png)

# R-shares

The contribution of a vote to a post (or comment) is measured in ```rshares```, and they are calculated from the vesting shares of curators (steem power) and the voting power used.

![6](http://quicklatex.com/cache3/07/ql_3a86a88ce1d6738211d55e3a7cb3fc07_l3.png)

where ```Vs``` are the vesting shares of the curator, and ```rs``` the rshares generated. The absolute value of ```rs``` must be greater than 50 VESTS, this is a dust threshold, the minimum amount.

# Total payout

There is a pool of rewards called ```reward_balance```. This balance is distributed between the latest posts. Then the payout of a single post, measured in STEEM, is calculated as follows:

![7](http://quicklatex.com/cache3/62/ql_46c02b684c92d4d8825befb0feaff062_l3.png)

where ```RS``` is the total rshares accumulated by the votes, ```rb``` is the ```reward_balance```, and ```rc``` is the ```recent_claims``` (measured in VESTS). This two values are global variables (look their values at [steemd.com](steemd.com)). If we want to calculate the payout in SBD, then the steem is multiplied by its price:

![8](http://quicklatex.com/cache3/33/ql_bdd4c44eeb40be53e41ab30fbfa02c33_l3.png)

# Worth of a vote

The worth of a vote is calculated in the same way as the total payout, but using the rshares of the vote. One thing to keep in mind is that a regular user does not know what their Vesting Shares are, but only knows their STEEM POWER. The way to calculate the them is using other 2 global variables, ```total_vesting_shares``` and ```total_vesting_fund_steem```:

![9](http://quicklatex.com/cache3/39/ql_be67ba7ac3d9719d4dba0dd34f442839_l3.png)

where ```Vs``` are the vesting shares of the curator, ```SP``` is the steem power, ```TVs``` is the ```total_vesting_shares```, and ```TVfs``` is the ```total_vesting_fund_steem```.

In this way, we can calculate the worth of a vote in a simplified way. Defining the voting power and the weight as values between 0 and 1 (0 for 0%, and 1 for 100%), the worth of a vote is:

![10](http://quicklatex.com/cache3/e6/ql_cf53d7dd2326a1cfc5b40686b70090e6_l3.png)

where ```Vp``` is the voting power, ```SP``` is the steem power, ```w``` is the weight of the vote, and ```g``` is a global variable calculated as:

![11](http://quicklatex.com/cache3/3c/ql_91e8278e0b958e5456077774d4f5b33c_l3.png)

where ```TVs``` is the ```total_vesting_shares```, ```TVfs``` is the ```total_vesting_fund_steem```, ```rb``` is the ```reward_balance```, ```rc``` is the ```recent_claims``` (measured in VESTS), and ```price``` is the steem price. All of these 5 values are global variables (look their values at [steemd.com](steemd.com)).

# Distribution of the total payout

The total payout is distributed between 3 parts: Author, curators, and beneficiaries. There is no a fixed percentage, but a series of calculations and transfers.

First, **75% are for the author**. However, in the post maybe a percentage of it is defined for the [beneficiaries](https://steemit.com/steemit/@heimindanger/split-the-rewards-of-your-posts-with-other-beneficiaries--only-on-steemwhalescom-). This is a new feature in the hard-fork 18 and allows new monetization options for developers working on Apps for STEEM.

## Distribution to curators

The remaining **25% of the payout are for the curators**. But some of their money goes back to the author, and the rest is finally distributed between curators. How is it calculated? First, lets define these values:

- ``` rs```: rshares generated when the curator votes.
- ```RS0```: rshares accumulated by the post before the curator votes.
- ```RS1```: rshares accumulated by the post after the curator votes. That is ```RS1 = RS0+rs```
- ```RST```: total rshares after a week

The percentage of reward for a single curator is:

![12](http://quicklatex.com/cache3/d8/ql_71835de1e37e450edfc8cefb91f081d8_l3.png)

However, some of this percentage goes back to the author. If the curator votes 30 minutes after the publication of the post, then 100% is for him, but if it is between the first 30 minutes he only takes a part of them, the ratio between the time and 30 min, and the rest is for the author. Then

![13](http://quicklatex.com/cache3/81/ql_1c8351c2d33e0b5da02efda13d500e81_l3.png)

where

![14](http://quicklatex.com/cache3/58/ql_cad2f3053f10d1da65544529c20f9d58_l3.png)
 
As we saw earlier, we can transform the rshares into STEEM or SBD with a global variable. Then if we multiply the numerator and denominator for the same value, we can calculate it in terms of STEEM or SBD. And next, multiplying it with 25% of the payout we can calculate the payout for a single curator:

![15](http://quicklatex.com/cache3/e5/ql_b673aea0e8aafafe3c8ede088f6ebee5_l3.png)

where ```P0``` is the payout of the post before voting, ```P1``` is the payout of the post after voting, and ```PT``` is the payout after 7 days (the final payout).

**Important note:** On steem, the formula to calculate the [square root is not exact but approximate](https://github.com/steemit/steem/blob/master/doc/sqrt.md), to be able to make operations fast. Then the above calculations could differ a bit from the reality. 

## Steem incentives voting good content

What is the incentive for being an early voter? Being ```P1``` the payout of the post after a curator votes, consider 2 situations:

1. After a week the post does not get more votes and stays the same. ```PT = P1```
2. After a week the payout scales ```n``` times, that is ```PT = n * P1```.

In the first case, the curator payout will be:

![16](http://quicklatex.com/cache3/15/ql_7b91c9fba639e2a570af0f829a7db115_l3.png)

And in the second case, the curator payout will be:

![17](http://quicklatex.com/cache3/68/ql_28275aa02f7ab3e5059f03fa83440b68_l3.png)

Then

![18](http://quicklatex.com/cache3/0d/ql_9a8b0bb00fd806c6969c297fedf9a30d_l3.png)

Then the incentive for being an early voter is ```sqrt(n)```. For instance, if the total payout scales 4 times, then the curator payout scales 2 times. If the total payout scales 9 times, the curator payout scales 3 times. The sooner the vote the more posibilities of scaling the curator reward (except the rule of 30 minutes).
