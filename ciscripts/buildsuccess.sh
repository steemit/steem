#/bin/bash
#testcommit
curl -XPOST -H "Authorization: token $GITHUB_SECRET" https://api.github.com/repos/steemit/steem/statuses/$(git rev-parse HEAD) -d "{
  \"state\": \"success\",
  \"target_url\": \"${BUILD_URL}\",
  \"description\": \"Jenkins-CI reports build succeeded!!\"
}"
rm -rf $WORKSPACE/*
