# k8deployer
An experimental deployer for kubernetes apps for developers who are too lazy (or busy) to learn Helm. 

## Status
**Early alpha.** 

The idea behind this project is to make a kubernetes app deployer for lazy developers. Lazy developers don't want to learn everything about kubernetes, we just want to deploy and test our application. Lazy developers don't want to repeat the same lame declarations in a zillion different sections in some yaml file. So it's kind of *Helm*, but simpler and capable of doing most of the behind the scenes stuff without detailed guidance.

K8deployer can deploy complex applications like Arangodb, without the need of operators. It can deploy or swap out single components in a complex app, for example to change a StatefulSet to a Deployment with Telepresence, making it simple to debug components locally on the developers laptop, while the component appears to the rest of the components as running in the kubernetes cluster.

## Design Requirements
- Simple deployment of k8s applications
- Minimum configuration. Let the deployer fill inn all the forms, using best practice patterns.
- Deploy to one or more k8s clusters in parallel.
- Support the most common k8s providers, so that a deployment can be deployed on any of them without changing the app configuration.
- Allow filtering and alternative components (to avoid commenting in and out blocks of declarations in the app yaml file).
- Live stream logs from started containers to the local machine.
- Allow single deployments to clusters in different environments (AWS, DO, Linode, Azure, BareMetal)
- Deal with public dns configuration, certificates, k8s ingress configuration
- Deal with proxying from the LoadBalancer / NodePort to the individual services
- Deal with RBAC
- Point a webbrowser (Firefox) to some url when some component is ready (just sip coffee and wait for the browser to open on your deployed app).

## Building

K8deployer is written in C++ and built with cmake.

It depends on these packages (Debian and Ubuntu):
```
  git build-essential zlib1g-dev cmake make libboost-all-dev libssl-dev g++
```

To build:
```sh
git clone https://github.com/jgaa/k8deployer.git
cd k8deployer
mkdir build
cd build
cmake ..
make -j `nproc`

```

### Build status
- **Debian Buster (10)**: OK
- **Ubuntu Focal (20.4 LTS)**: OK
- **Ubuntu Bionic (18.04 LTS)**: Require compiler upgrade to gcc 9, and a newer boost library. You must make g++-9 default before building. Tested OK with g++-9 and libboost 1.75. [More details](doc/ubuntu-bionic.md)

