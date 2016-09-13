The git guidelines for Steemit are influenced by the
[Graphene](https://github.com/cryptonomex/graphene/wiki/How-we-use-version-control)
git guidelines as well as [Git
Flow](http://nvie.com/posts/a-successful-git-branching-model/) and [this
blog
post](http://www.draconianoverlord.com/2013/09/07/no-cherry-picking.html).

## Branches

- `origin/master`  : Contains releases of Steem. `origin/master` and
  `origin/HEAD` should always be at the most recent release. All witnesses
  should be running this branch. Release commits will have a label
  `vMajor.Hardfork.Release`. When we get ready to release we will merge
  feature branches into develop and then do a single merge into master. Pull
  Requests must pass automated testing to be merged into master.
- `origin/develop` : The development branch. We will strive to keep develop
  as a working branch as much as possible. All branches must pass automated
  tests before being merged. While this is not a guarantee that develop is
  bug free, it will guarantee to the best of our knowledge that it is safe
  to build from develop. That being said, running a node from develop has
  risks. We recommend that any block producing node build from master. If
  you want to test new features, develop is the correct branch.
- `origin/staging` : The purpose of staging was to be a more stable version
  of develop. However, with the requirement to pass automated testing to
  merge into develop, the purpose of staging is not longer relevent. This
  branch is now defunct.

### Patch Branches

All issues should be developed against their own branch. These branches
should base off `develop` and then merged back into develop when they are
tested and ready to be deployed. If an issue needs another issue as a
dependency, branch from `develop`, merge the dependent issue branch into the
new branch, and begin development. The naming scheme we use is the issue
number, then a dash, followed by a shorthand description of the issue. For
example, issue 22 is to allow the removal of an upvote. Branch
`22-undo-vote` was used to devlop the patch for this issue.

### Non-Issue Branches

Some changes are so minor as to not require an issue, e.g. changes to
logging. Because the requirement of automated testing, create an issue for
them. We air on the side of overdocumentation rather than
underdocumentation. Branch from `develop`, make your fix, and create a pull
request into `develop`.

## Pull Requests

All merges into `develop` and `master` are done through pull requests. This
is done for several reasons:

1. It enforces two factor authentication. Github will only allow merging of a
   pull request through their interface, which requires the dev to be logged
   in.
1. If enforces testing. All pull requests undergo automated testing they are
   allowed to be merged.
1. It enforces best practices. Becuase of the cost of a pull request,
   developers are encouraged to do more testing themselves and be certain of
   the correctness of their solutions.
1. If enforces code review. All pull request must be reviewed by a developer
   other than the creator of the request. Pull requests made by external
   developers should be reviewed by two internal developers. When a developer
   reviews and approves a pull request they should +1 the request or leave a
   comment mentioning their approval of the request. Otherwise they should
   describe what the problem is with the request so the developer can make
   changes and resubmit the request.

All pull requests should reference the issue(s) they relate to in order to
create a chain of documentation.

If your pull request has merge conflicts, rebase against `origin/develop`,
resolve the merge conflicts, force push to your branch, and resubmit the
pull request.

## Policies

### Force-push policy

- `origin/master` should never be force pushed.
- `origin/develop` should never be force pushed. All updates to `develop`
  are done through pull requests so force pushing is not allowed.
- Individual patch branches may be force-pushed at any time, at the
  discretion of the developer or team working on the patch.

### Tagging Policy

- Tags are reserved for releases. The version scheme is
  `vMajor.Hardfork.Release` (Ex. v0.5.0 is the version for the Hardfork 5
  release). Releases should be made only on `master`.

### Code review policy

- Two developers *must* review *every* release before merging it into
  `origin/master`, enforced through pull requests.
- Two developers *must* review *every* consensus-breaking change before it
  moves into `origin/develop`, enforced through pull requests.
- Two developers *should* review *every* patch before it moves into
  `orogin/develp`, enforced through pull requests.
- One of the reviewers may be the author of the change.
- This policy is designed to encourage you to take off your "writer hat" and
  put on your "critic/reviewer hat."  If this was a patch from an unfamiliar
  community contributor, would you accept it?  Can you understand what the
  patch does and check its correctness based only on its commit message and
  diff?  Does it break any existing tests, or need new tests to be written?
  Is it stylistically sloppy -- trailing whitespace, multiple unrelated
  changes in a single patch, mixing bugfixes and features, or overly verbose
  debug logging?
- Having multiple people look at a patch reduces the probability it will
  contain bugs.
