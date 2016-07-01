The git guidelines for Steemit are influenced by the [Graphene](https://github.com/cryptonomex/graphene/wiki/How-we-use-version-control) git guidelines as well as [Git Flow](http://nvie.com/posts/a-successful-git-branching-model/) and [this blog post](http://www.draconianoverlord.com/2013/09/07/no-cherry-picking.html).

### Branches

- `origin/master`  : Contains releases of Steem. `origin/master` and `origin/HEAD` should always be at the most recent release. All witnesses should be running this branch.
- `origin/staging` : Staging branch for new releases. Release candidates of develop will be merged into staging for review. Witnesses wanting to run the latest builds and test new features and bug fixes should be on this branch.
- `origin/develop` : The development branch. Develop should always build but may contain bugs. This branch is only for developers.

### Patch Branches

All issues should be developed against their own branch. These branches should have the most recent staging branch as a working parent and then merged into develop when they are tested and ready to be deployed.
If an issue needs another issue as a dependency, branch from `staging`, merge the dependent issue branch into the new branch, and begin development. The naming scheme we use is the issue number, then a dash, followed by a shorthand description of the issue. For example, issue 22 is to allow the removal of an upvote. Branch `22-undo-vote` was used to devlop the patch for this issue.

### Non-Issue Branches

Some changes are so minor as to not require an issue, e.g. changes to logging. If in doubt, create an issue for the change and follow the guidelines above. Issues serve as documentation for many changes. These should still be done against `staging` where possible (or the earliest commit they depend on, where not possible) and merged into `develop`.  In practice you may develop such patches against `master`; then rebase against `develop` before pushing.

##Policies

### Force-push policy

- `origin/master` should never be force pushed.
- `origin/staging` should rarely be force pushed. Exceptions to this policy may be made on a case-by-case basis.
- `origin/develop` should rarely be force pushed. It may be force-pushed to kill patches that are prematurely merged. Force pushing beyond this is likely an idication development should have occured on an issue branch.
- Individual patch branches may be force-pushed at any time, at the discretion of the developer or team working on the patch.

### Tagging Policy

- Tags are reserved for releases. The version scheme is `vMajor.Hardfork.Release` (Ex. v0.5.0 is the version for the Hardfork 5 release). Releases should be made only on `master`.

### Code review policy

- Two developers *must* review *every* consensus-breaking change before it moves into `graphene/master`.
- Two developers *should* review *every* patch before it moves into `graphene/master`.
- One of the reviewers may be the author of the change.
- This policy is designed to encourage you to take off your "writer hat" and put on your "critic/reviewer hat."  If this was a patch from an unfamiliar community contributor, would you accept it?  Can you understand what the patch does and check its correctness based only on its commit message and diff?  Does it break any existing tests, or need new tests to be written?  Is it stylistically sloppy -- trailing whitespace, multiple unrelated changes in a single patch, mixing bugfixes and features, or overly verbose debug logging?
- Having multiple people look at a patch reduces the probability it will contain bugs.