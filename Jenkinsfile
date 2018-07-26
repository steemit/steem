#!groovy
pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        parallel ( "Build tests":
        {
          sh 'ciscripts/triggertests.sh'
        },
        "Build docker image": {
          sh 'ciscripts/triggerbuild.sh'
        })
      }
    }
  }
  post {
    success {
      sh 'ciscripts/buildsuccess.sh'
    }
    failure {
      sh 'ciscripts/buildfailure.sh'
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}
