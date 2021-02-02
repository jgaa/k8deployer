# k8deployer
For now, an experimental kubernetes deployer in C++

## Requiremeents
- Simple deployment of k8 applications
- Minimum configuration. Let the deployer fill inn all the forms, using best practice patterns.
- Deploy to one or more k8 clusters in parallell
- Support the most common k8 providers, so that a deployment can be deployed on any of them without changing the app configuration
- Allow single deployments to clusters in different environments (AWS, DO, Linode, Azure, BareMetal)
- Optional: Deal with public dns configuration, certificates, external load balancers
- Optional: Deal with proxying from the LoadBalancer / NodePort to the individual services
- Optional: Deal with RBAC

## Todo for initial beta

- [ ] Deployment
    - [x] Create
        - [ ] Environment variables from args (key=value,...)
        - [ ] Environment variables from ConfigMap
    - [x] Delete
    - [ ] Update
    - [ ] Verify

- [ ] Service
    - [x] Create
    - [x] Delete
    - [ ] Update
    - [ ] Verify

- [ ] Configmap
    - [ ] Create from configuration
    - [x] create from file
    - [x] Delete
    - [ ] Update
    - [ ] Verify

- [ ] Secrets
    - [x] Docker repositoiry secrets
        - [x] Use in podspec
    - [ ] Optimize secrets, so we only have one object per actual secret
    - [ ] Change protocol to https:// (directly to cluster) in order to create secrets

- [ ] StatefulSet
    - [x] Create
        - [x] Deal with storage
            - [x] Local
            - [x] Network
                - [x] nfs
    - [x] Delete
        - [x] Deal with storage
        - [ ] Optionally, delete the data (if possible)
    - [ ] Update
    - [ ] Verify

- [ ] Namespaces
    - [ ] Create
    - [ ] Delete

- [ ] Storage reservation pools / automatoc VolumeMounts
    - [x] Support NFS, manual and automatically
    - [x] Support single node local storage
    - [ ] Support AWS
    - [ ] Support GCP
    - [ ] Support Azure
    - [ ] Support GlusterFS
    - [ ] Support DigitalOcean (?)
    - [ ] Support Single Node bare metal

- [x] Event-driven work-flow.
    - [x] Relate events to components.
    - [x] Update component state from events
    - [x] Trigger next action(s) from state changes

- [ ] Error handling
    - [ ] Fail fast on errors, stop deployments and try to roll back
    - [ ] Abort on connectivity issues for now
    - [ ] For undeploy, add option to ignore errors and try to continue.

## Bugs / limitations
- [x] App need to exit when the execution is complete
- [x] Add readyness probe for pods
- [x] Add alive probe for pods
- [x] deployment.spec.affinity.nodeAffinity must be std::optional (needs support in the json serializor)
