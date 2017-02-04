#!groovy
node {
  stage 'Checkout'
  checkout scm
  properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])]) // required to enable webhooks for a job
  stage 'Build'
  try {
    sh 'ciscripts/triggerbuild.sh'
    sh 'ciscripts/buildsuccess.sh'
  } catch (err) {
    currentBuild.results = 'FAILURE'
    sh 'ciscripts/buildfailure.sh'
  }
}
