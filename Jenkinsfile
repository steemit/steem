#!groovy
node {
  stages {
    stage('Build') {
      steps {
        properties([pipelineTriggers([[$class: 'GitHubPushTrigger'], pollSCM('H/1 * * * *')])])
        checkout scm
        sh 'ciscripts/triggerbuild.sh'
      }
    }
  }
  post {
    success {
      sh 'ciscripts/buildsuccess.sh'
    }
    failure {
      sh 'ciscripts/buildfailure.sh'
    }
  }
}
