# Buiding on Ubuntu 18.04

This version is really old, and it is time to upgrade. But if you prefer to wait a bit with the upgrade, there are (at least) two ways to build this C++ project.

## When you don't use C++ and don't care

You need to upgrade g++ and build and install a newer version of the boost library.

```
# Install required packages and boost

aapt-get -q update &&\
    apt-get -y -q --no-install-recommends upgrade &&\
    apt-get -y -q install git \
    build-essential apt-utils \
    software-properties-common curl \
    zlib1g-dev make libssl-dev &&\
    add-apt-repository -y ppa:ubuntu-toolchain-r/test &&\
    apt-get -q update &&\
    apt-get -y -q install gcc-9 g++-9 &&\
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-9 50 &&\
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-9 50 &&\
    mkdir -p /tmp/boost && curl -sL https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2 | tar -xjC /tmp/boost --strip-components 1 &&\
    cd /tmp/boost && ./bootstrap.sh && ./b2 toolset=gcc-9 install && cd && rm -rf /tmp/boost
    
# Build the project

cd && git clone https://github.com/jgaa/k8deployer.git &&\
mkdir -p k8deployer/build &&\
cd k8deployer/build &&\
cmake -DBOOST_ROOT=/opt/boost_1_75 .. &&\
make -j `nproc`
./k8deployer -h

```

## When you use C++ and can't install a new boost dev library

```
apt-get -q update &&\
    apt-get -y -q --no-install-recommends upgrade &&\
    apt-get -y -q install git \
    build-essential apt-utils \
    software-properties-common curl \
    zlib1g-dev make libssl-dev &&\
    add-apt-repository -y ppa:ubuntu-toolchain-r/test &&\
    apt-get -q update &&\
    apt-get -y -q install gcc-9 g++-9 &&\
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-9 50 &&\
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-9 50 &&\
    mkdir -p /opt/boost_1_75 && curl -sL https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2 | tar -xjC /opt/boost_1_75 --strip-components 1 &&\
    cd /opt/boost_1_75 && ./bootstrap.sh && ./b2 toolset=gcc-9 stage

cd && git clone --depth 1 https://github.com/Kitware/CMake.git && cd CMake && ./bootstrap && ./configure && make -j `nproc` && make install

cd && git clone https://github.com/jgaa/k8deployer.git &&\
mkdir -p k8deployer/build &&\
cd k8deployer/build &&\
cmake -DBOOST_ROOT=/opt/boost_1_75 .. &&\
make -j `nproc`

LD_LIBRARY_PATH=/opt/boost_1_75/stage/lib/ ./k8deployer -h

```
