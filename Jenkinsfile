
def user_name
def user_id
def group_id
def workdir
def linuxDockerfile;
def doWindowsBuild;

node {
  user_name = sh(returnStdout: true, script: 'id -un').trim()
  user_id = sh(returnStdout: true, script: 'id -u').trim()
  group_id = sh(returnStdout: true, script: 'id -g').trim()
  workdir = pwd()
  builddir = "${workdir}/insight-build"
}

pipeline {
   
 agent none
 
 stages {
 
  stage('retrieveConfig') {
    agent any
    steps {
     script {
        linuxDockerfile = sh(returnStdout: true, script: "bash -c \"source ${workdir}/configuration.sh; echo \\\$DOCKERFILE\"").trim()
        doWindowsBuild = sh(returnStdout: true, script: "bash -c \"source ${workdir}/configuration.sh; echo \\\$DO_WINDOWS_BUILD\"").trim()
     }
    }
  }
  
  stage('Prepare') {
   agent any 
   steps {
    sh "mkdir -p ${builddir} insight-windows-build mxe"
    sh "rm -vf ${builddir}/insightcae*.deb"
    sh "rm -vf ${builddir}/insightcae*.rpm"
    sh "rm -vf insight-windows-build/InsightCAEInstaller*"
    sh "rm -vf ${builddir}/insightcae*.tar.gz"
    sh "FILES=`find ${workdir} -iname *.i`; if [ -n \"\\\$FILES\" ]; then touch \\\$FILES; fi"
  } }

  stage('Linux-Build') {

   agent { dockerfile {
    filename "${linuxDockerfile}"
    dir "buildsystems"
    additionalBuildArgs  "--build-arg USER=${user_name} --build-arg UID=${user_id} --build-arg GID=${group_id}"
    args "-v ${workdir}:/insight-src"
   } }
        
   steps {
    sh './run.sh build'
    sh './run.sh TGZ'
      archiveArtifacts artifacts: "${builddir}/*.tar.gz", fingerprint: true
    sh './run.sh package'
      archiveArtifacts artifacts: "${builddir}/*.deb, ${builddir}/*.rpm", fingerprint: true
    }
  }
  

  // !!! Windows build after Linux superbuild: 
  // we need to access the insight git repo which was updated during Linux build
  // and the pdl (linux) binary
  stage('WSL-Image') {
   when {
     expression {
       return doWindowsBuild == '1';
     }
   }
   agent any
   steps {
    sh './wslimage.sh'
      archiveArtifacts artifacts: 'wsl/insightcae-ubuntu-*.tar.gz', fingerprint: true
   }
  }

  stage('Windows-Build') {
  
   when {
     expression {
       return doWindowsBuild == '1';
     }
   }
   
   agent { dockerfile {
    filename 'insightcae-buildsystem_windows.docker'
    dir "buildsystems"
    additionalBuildArgs  "--build-arg USER=${user_name} --build-arg UID=${user_id} --build-arg GID=${group_id}"
    args "-v ${workdir}:/insight-src -v ${workdir}/insight-windows-build:/insight-windows-build"
   } }
        
   steps {
    sh './mxe.sh build'
    sh './mxe.sh package'
      archiveArtifacts artifacts: "${workdir}/insight-windows-build/InsightCAEInstaller*.exe", fingerprint: true
   }
  }
  
  stage('Distribution') {
   agent any
   steps {
    sh "./updateRepos.sh"
   }
  }

 }
 
 post {

  success {
      mail to: 'hannes@kroegeronline.net',
        subject: "Successfully completed ${currentBuild.fullDisplayName}",
        body: "Success, see ${currentBuild.absoluteUrl}"
  }
  
  failure {
      node(null) {
       archiveArtifacts artifacts: " ${builddir}/insight/stamp/insight-build.log", fingerprint: true
      }
      
      mail to: 'hannes@kroegeronline.net',
        subject: "Failed to build ${currentBuild.fullDisplayName}",
        body: "Failure, see ${currentBuild.absoluteUrl}"
  }
  
 }

 
}
