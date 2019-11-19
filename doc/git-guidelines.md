## Branches
- `stable`: Points to the latest version of code that is production-ready, and has been tested in production. This is the branch we recommend for exchanges.
- `0.22.x`: This is the current release branch and will always point to the most recent release. Witnesses and seed node operators may want to run this branch. `stable` and `0.22.x` may diverge if there are minor bug fixes or witness only security fixes that does not need exchanges to update for. 
- `master`: The active development branch. We will strive to keep `master` in a working state. All PRs must pass automated tests before being merged. While this is not a guarantee that `master` is bug-free, it will guarantee that the branch is buildable in our standard build configuration and passes the current suite of tests. That being said, running a node from `master` has risks.  We recommend that any block producing node build from `stable` or `0.22.x`. If you want to test new features, `master` is the correct branch.

### Releases

There are two release procedures depending on if the release is a major or minor release. If the release is a minor release, the release branch (`0.22.x`) will be tagged and merged in to `master`. This ensures that all changes in the release get merged to `master`. If the release is a major release, the release branch will be merged in to master (in case there are unreleased changes), a new release branch will be branched from `master` (i.e. `0.22.x`) and the release tagged on the new release branch.

### Patch Branches

All changes should be developed in their own branch. These branches should base off either `master` or `0.22.x` and then merged back into the respective base branch when they are tested and ready. The naming scheme we use is the issue number, then a dash, followed by a shorthand description of the issue. For example, issue 22 is to allow the removal of an upvote. Branch `22-undo-vote` was used to develop the patch for this issue.

### Non-Issue Branches

Some changes are so minor as to not require an issue, e.g. changes to logging. Because the requirement of automated testing, create an issue for them. We err on the side of over-documentation rather than under-documentation.  Branch from `master`, make your fix, and create a pull request into `master`.

Suggested formatting for such branches missing an issue is `YYYYMMDD-shortname`, e.g. `20160913-documentation-update`.  (The date in the branch is so that we can prune old/defunct ones periodically to keep the repo tidy.)

## Pull Requests

All changes to `0.22.x` and `master` are done through GitHub Pull Requests (PRs). This is done for several reasons:

- It enforces two factor authentication. GitHub will only allow merging of a pull request through their interface, which requires the dev to be logged in.
- It enforces testing. All pull requests undergo automated testing before they are allowed to be merged.
- It enforces best practices. Because of the cost of a pull request, developers are encouraged to do more testing themselves and be certain of the correctness of their solutions.
- It enforces code review. All pull requests must be reviewed by a developer other than the creator of the request. Pull requests made by external developers should be reviewed by two internal developers. When a developer reviews and approves a pull request they should +1 the request or leave a comment mentioning their approval of the request. Otherwise they should describe what the problem is with the request so the developer can make changes and resubmit the request.

All pull requests should reference the issue(s) they relate to in order to create a chain of documentation.

If your pull request has merge conflicts, merge the head of the base branch in to feature branch to resolve the merge conflicts locally and push the changes to use the same PR. A rebase can be used, but merging from the base branch is preferred because history can be lost in a rebase and it is easy to perform a rebase incorrectly and lose important code from the feature branch.

### Community Pull Requests

Community members can submit remote PRs to the Steem repo. It is the responsibility of the community developer to create the PR against the correct base branch (`0.22.x` or `master`). PRs with merge conflicts or excessive commits due to merging in the wrong direction will automatically be closed and it will be the responsibility of the community developer to fix their branch and resubmit their pull request.

Once the community PR has been reviewed, a Steemit developer will merge the community PR to a branch named after the PR issue number followed by `-PR` to denote the branch is a community submission. For example, the local branch for community PR #3412 was named `3412-PR`. A new PR is then created, which links back the community PR and quotes any relevant conversation from the community PR. The normal code review process is still followed, requiring another Steemit developer to review the PR before merging. The developer approving the community PR is vouching for the validity of the remote PR as though it were their own code.

The community PR runs through our same CI testing as our code once it has been merged to the local branch. Steemit reserves the right to close the PR at this point and request changes from the community developer if the branch fails CI. Depending on the reason for the failure and the importance of the branch, our developers may fix it themselves at their own discretion. To help prevent situations like this from happening, we recommend that all community developers build and test locally. You can run `docker build .` in the root directory of the project to build the project in a Docker container that mirrors our CI environment.

## Policies

### Force-Push Policy

- In general, protected branches, `stable`, `v0.22.x`, and `master` must never be force pushed except when setting them to the proper commits during a release. This is enforced via GitHub's branch protection.
- Individual patch branches may be force-pushed at any time, at the discretion of the developer or team working on the patch. Do not force push another developer's branch without their permission.

### Tagging Policy

- Tags are reserved for releases. The version scheme is `vMajor.Hardfork.Release` (Ex. v0.5.0 is the version for the Hardfork 5 release).

### Code Review Policy / PR Merge Process

- Two developers *must* review *every* change before it moves into `master` or `0.22.x`, enforced through pull requests. Seemingly benign changes can have profound impacts on consensus. Every change should be treated as a potential consensus breaking change.
- One of the reviewers may be the author of the change.
- This policy is designed to encourage you to take off your "writer hat" and put on your "critic/reviewer hat."  If this were a patch from an unfamiliar community contributor, would you accept it?  Can you understand what the patch does and check its correctness based only on its commit message and diff? Does it break any existing tests, or need new tests to be written? Is it stylistically sloppy -- trailing whitespace, multiple unrelated changes in a single patch, mixing bug fixes and features, or overly verbose debug logging?
- Having multiple people look at a patch reduces the probability it will contain uncaught bugs.
- Community developers are encouraged to leave reviews on PRs but are not required to, nor must their suggestions be followed. However, we have had community developers catch bugs for us during review. These reviews are not law, but are often helpful and should not be ignored.

## Example Release Branch Management 

This example is based on release `0.21.0`. Future releases work the same way, but the name of the release branch will be different.

1. New development is done on master
```
master
     *
     |
     |
     |
```

2. A release is made
```
master/v0.21.0
     *
     |
     |
     |
```

3. New development continues
```
master
     *
     |
     *  <- 0.21.x (branch), stable (branch), v0.21.0 (tag)
     |
     |
```

4. A minor patch is required on 0.21.0
```
master
     *
     |   *  <- 0.21.x (branch), v0.21.1 (tag)
     |  /
     | /
     *  <- stable (branch), v0.21.0 (tag)
     |
     |
```

5. Keep master updated with fixes
```
master
     *
     | \
     |  \
     |   *  <- 0.21.x (branch), v0.21.1 (tag)
     |  /
     | /
     *  <- stable (branch), v0.21.0 (tag)
     |
     |
```

6. And so on...
```
                   master
fixes merged->     *
                   | \
                   |  \
                   |   \
                   |    \
                   |     \
fixes merged->     |      * <- 0.20.x (branch), 0.21.2 (tag)
                   | \   /
                   |  \ /
                   |   *  <- v0.21.1 (tag)
                   |  /
                   | /
                   *  <- stable (branch), v0.21.0 (tag)
                   |
                   |
```
