#!groovy
node {
  try {
    properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])])
    checkout scm
    sh 'ciscripts/triggerbuild.sh'
    sh 'ciscripts/buildsuccess.sh'
  } catch (err) {
    currentBuild.results = 'FAILURE'
    sh 'ciscripts/buildfailure.sh'
  }
}
