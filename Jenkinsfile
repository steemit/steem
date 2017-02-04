#!groovy
node {
  try {
    properties([pipelineTriggers([[$class: 'GitHubPushTrigger'], pollSCM('H/2 * * * *')])])
    checkout scm
    sh 'ciscripts/triggerbuild.sh'
    sh 'ciscripts/buildsuccess.sh'
  } catch (err) {
    currentBuild.results = 'FAILURE'
    sh 'ciscripts/buildfailure.sh'
  }
}
