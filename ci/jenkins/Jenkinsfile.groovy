#!/usr/bin/env groovy

pipeline {
    agent { label 'master' }

    stages {
        stage('Build') {
           parallel {
                stage('Ubuntu Bionic') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-bionic'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on ubuntu-bionic-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'
                    }
                }

                
                stage('Debian Buster') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.debian-buster'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on debian-buster-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'
                    }
                }

                
                stage('Ubuntu Focal') {
                    agent {
                        dockerfile {
                            filename 'Dockefile.ubuntu-focal'
                            dir 'ci/jenkins'
                            label 'docker'
                        }
                    }

                    steps {
                        echo "Building on ubuntu-focal-AMD64 in ${WORKSPACE}"
                        checkout scm
                        sh 'pwd; ls -la'
                        sh 'rm -rf build'
                        sh 'mkdir build'
                        sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'
                    }
                }
                
                
//                 stage('Debian Testing') {
//                     agent {
//                         dockerfile {
//                             filename 'Dockefile.debian-testing'
//                             dir 'ci/jenkins'
//                             label 'docker'
//                         }
//                     }
// 
//                     steps {
//                         echo "Building on debian-testing-AMD64 in ${WORKSPACE}"
//                         checkout scm
//                         sh 'pwd; ls -la'
//                         sh 'rm -rf build'
//                         sh 'mkdir build'
//                         sh 'cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)'
// 
//                         echo 'Getting ready to run tests'
//                         script {
//                             try {
//                                 sh 'cd build && ctest --no-compress-output -T Test'
//                             } catch (exc) {
//                                 echo 'Testing failed'
//                                 currentBuild.result = 'UNSTABLE'
//                             }
//                         }
//                     }
//                 }


            }
        }
    }
}

