The git guidelines for SteemIt are influenced by the
[Graphene](https://github.com/cryptonomex/graphene/wiki/How-we-use-version-control)
git guidelines as well as [Git
Flow](http://nvie.com/posts/a-successful-git-branching-model/) and [this
blog
post](http://www.draconianoverlord.com/2013/09/07/no-cherry-picking.html).

## Branches

- `master`: Points to the current release of Steem.  Witnesses should be
  running this branch. Each release commit will be tagged
  `vMajor.Hardfork.Release`. When we get ready to release we will merge
  feature branches into `develop` and then do a single merge into `master`
  via a Pull Request. All PRs to `master` must pass automated testing to be
  merged.
- `develop`: The active development branch. We will strive to keep `develop`
  in a working state. All PRs must pass automated tests before being merged.
  While this is not a guarantee that `develop` is bug-free, it will
  guarantee that the branch is buildable in our standard build configuration
  and passes the current suite of tests. That being said, running a node
  from `develop` has risks.  We recommend that any block producing node
  build from `master`. If you want to test new features, develop is the
  correct branch.

### Patch Branches

All changes should be developed in their own branch. These branches
should branch from `develop` and then merged back into `develop` when they are
tested and ready. If an issue needs another issue as a
dependency, branch from `develop`, merge the dependent issue branch into the
new branch, and begin development. The naming scheme we use is the issue
number, then a dash, followed by a shorthand description of the issue. For
example, issue 22 is to allow the removal of an upvote. Branch
`22-undo-vote` was used to develop the patch for this issue.

### Non-Issue Branches

Some changes are so minor as to not require an issue, e.g. changes to
logging. Because the requirement of automated testing, create an issue for
them. We err on the side of over-documentation rather than
under-documentation.  Branch from `develop`, make your fix, and create a pull
request into `develop`.

Suggested formatting for such branches missing an issue is
`YYYYMMDD-shortname`, e.g. `20160913-documentation-update`.  (The date in
the branch is so that we can prune old/defunct ones periodically to keep the
repo tidy.)

## Pull Requests

All changes to `develop` and `master` are done through GitHub Pull Requests
(PRs). This is done for several reasons:

- It enforces two factor authentication. GitHub will only allow merging of a
  pull request through their interface, which requires the dev to be logged
  in.
- If enforces testing. All pull requests undergo automated testing before
  they are allowed to be merged.
- It enforces best practices. Because of the cost of a pull request,
  developers are encouraged to do more testing themselves and be certain of
  the correctness of their solutions.
- If enforces code review. All pull requests must be reviewed by a developer
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

### Force-Push Policy

- `origin/master` should never be force pushed.
- `origin/develop` should never be force pushed. All updates to `develop`
  are done through PRs so force pushing is not allowed.
- Individual patch branches may be force-pushed at any time, at the
  discretion of the developer or team working on the patch.

### Tagging Policy

- Tags are reserved for releases. The version scheme is
  `vMajor.Hardfork.Release` (Ex. v0.5.0 is the version for the Hardfork 5
  release). Releases should be made only on `master`.

### Code Review Policy / PR Merge Process

- Two developers *must* review *every* release before merging it into
  `master`, enforced through pull requests.
- Two developers *must* review *every* consensus-breaking change before it
  moves into `develop`, enforced through pull requests.
- Two developers *should* review *every* patch before it moves into
  `develop`, enforced through pull requests.
- One of the reviewers may be the author of the change.
- This policy is designed to encourage you to take off your "writer hat" and
  put on your "critic/reviewer hat."  If this were a patch from an
  unfamiliar community contributor, would you accept it?  Can you understand
  what the patch does and check its correctness based only on its commit
  message and diff? Does it break any existing tests, or need new tests to
  be written? Is it stylistically sloppy -- trailing whitespace, multiple
  unrelated changes in a single patch, mixing bug fixes and features, or
  overly verbose debug logging?
- Having multiple people look at a patch reduces the probability it will
  contain uncaught bugs.
