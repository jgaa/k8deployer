# Todo for initial beta

- [ ] Deployment
    - [x] Create
        - [x] Environment variables from args (key=value,...)
        - [ ] Environment variables from ConfigMap
        - [ ] Environment variables from Secrets
        - [x] Apply Resource limits
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
        - [x] Read from docker's config file
    - [x] TLS secret (read crt/key from pem files)
    - [ ] Optimize secrets, so we only have one object per actual secret
    - [x] Change protocol to https:// (directly to cluster) in order to create secrets
    - [ ] Generate passwords and store them in secrets during deployment
        - [ ] Glue to environment variables 

- [ ] StatefulSet
    - [x] Create
        - [x] Deal with storage
            - [x] Local
            - [x] Network
                - [x] nfs
        - [x] Apply Resource limits
    - [x] Delete
        - [x] Deal with storage
        - [ ] Optionally, delete the data (if possible)
    - [ ] Update
    - [ ] Verify
    
- [x] Ingress
    - [x] Hosts
    - [x] Paths
    - [x] TLS / Certs

- [ ] DaemonSet
    - [x] Create
        - [x] Apply Resource limits
    - [x] Delete
    - [ ] Update
    - [ ] Verify

- [ ] Namespaces
    - [x] Create
    - [x] Delete

- [ ] Storage reservation pools / automatoc VolumeMounts
    - [x] Support NFS, manual and automatically
    - [x] Support single node local storage
    - [x] Support automatic claim providers (tested with minikube)
    - [x] Support for sig Local Persistence Volume Static Provisioner
    - [ ] Support AWS
    - [ ] Support GCP
    - [ ] Support Azure
    - [ ] Support GlusterFS
    - [ ] Support DigitalOcean (?)

- [x] Event-driven work-flow.
    - [x] Relate events to components.
    - [x] Update component state from events
    - [x] Trigger next action(s) from state changes

- [ ] RBAC
    - [ ] Create roles
    - [ ] Refer components to roles

- [ ] Error handling
    - [ ] Fail fast on errors, stop deployments and try to roll back
    - [ ] Abort on connectivity issues for now
    - [x] For undeploy, ignore errors and try to continue.

## Bugs / limitations
- [x] App need to exit when the execution is complete
- [x] Add readyness probe for pods
- [x] Add alive probe for pods
- [x] deployment.spec.affinity.nodeAffinity must be std::optional (needs support in the json serializor)
