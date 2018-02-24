#!groovy
pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'ciscripts/triggerbuild.sh'
      }
    }
  }
  post {
    always {
      step([$class: 'CoberturaPublisher', autoUpdateHealth: false, autoUpdateStability: false, coberturaReportFile: '/var/jenkins_home/cobertura/coverage.xml', failUnhealthy: false, failUnstable: false, maxNumberOfBuilds: 0, onlyStable: false, sourceEncoding: 'ASCII', zoomCoverageChart: false])
    }
    success {
      sh 'ciscripts/buildsuccess.sh'
    }
    failure {
      sh 'ciscripts/buildfailure.sh'
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}
