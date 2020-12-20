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
    - [ ] Docker repositoiry secrets
        - [ ] Use in podspec

- [ ] StatefulSet
    - [ ] Create
        - [ ] Deal with storage
            - [ ] Local
            - [ ] Network
                - [ ] nfs
    - [ ] Delete
        - [ ] Deal with storage
    - [ ] Update
    - [ ] Verify

- [ ] Namespaces
    - [ ] Create
    - [ ] Delete

- [ ] Storage reservation pools
    - [ ] Figure out what to do

- [ ] Event-driven work-flow.
    - [x] Relate events to components.
    - [x] Update component state from events
    - [x] Trigger next action(s) from state changes

- [ ] Error handling
    - [ ] Fail fast on errors, stop deployments and try to roll back
    - [ ] Abort on connectivity issues for now
    - [ ] For undeploy, add option to ignore errors and try to continue.

## Bugs / limitations
- [ ] App need to exit when the execution is complete
- [ ] Add readyness probe for pods
- [ ] Add alive probe for pods
- [ ] deployment.spec.affinity.nodeAffinity must be std::optional (needs support in the json serializor)
