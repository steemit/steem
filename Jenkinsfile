pipeline {
  agent any

  stages {
    stage('Build') {
      steps {
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
