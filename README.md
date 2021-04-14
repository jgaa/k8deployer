# k8deployer
For now, an experimental kubernetes deployer in C++

The idea behind this project is to make a kubernetes app deployer for lazy developers. Lazy developers don't want to learn everything about kubernetes, we just want to deploy and test our application. Lazy developers don't want to repeat the same lame declarations in a zillion different sections in some yaml file. So it's kind of *Helm*, but more mature and able to do most of the behind the scenes stuff without detailed guidance.

K8deplyer can deploy complex applications like Arangodb, without the need of operators. It can deploy or swap out single components in a complex app, for example to change a StatefulSet to a Deployment with Telepresence, making it simple to debug components locally on the developers laptop, while the component appears to the rest of the components as running in the kubernetes cluster.

## Requirements
- Simple deployment of k8 applications
- Minimum configuration. Let the deployer fill inn all the forms, using best practice patterns.
- Deploy to one or more k8 clusters in parallel
- Support the most common k8 providers, so that a deployment can be deployed on any of them without changing the app configuration
- Allow filtering and alternative components (to avoid commenting in and out blocks of declarations in the app yaml file)
- Live stream logs from started containers to the local machine
- Allow single deployments to clusters in different environments (AWS, DO, Linode, Azure, BareMetal)
- Optional: Deal with public dns configuration, certificates, external load balancers
- Optional: Deal with proxying from the LoadBalancer / NodePort to the individual services
- Optional: Deal with RBAC

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
- **Ubuntu Bionic (18.04 LTS)**: Broken

