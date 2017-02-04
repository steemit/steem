#!groovy
pipeline {
  agent any

  stages {
    stage('Build') {
      steps {
        properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])])
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
