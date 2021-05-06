# Reference documentation for K8deployer


## Components

K8deployer is based on a set of component, mostly based on k8s components. Unlike most other k8s deployment tools (like `kubectl apply` or `helm`), 
k8deployer creates most of the components itself, based on the arguments for the target components, like Deployment and StatefulSet.

|Kind               |Purpose     |
|-------------------|------------|
|App                |Placeholder for one or more child-components. This is a deploy-construct to group related components. Other components can depend on the App; effectively keeping them pending until all the App's components are in running state.|
|ClusterRole        |A k8s ClusterRole|
|ClusterRoleBinding |A k8s ClusterRoleBinding|
|ConfigMap          |A k8s ConfigMap. These are normally created automatically based on the arguments to Deployment, StateFulSet and DaemonSet components|
|DaemonSet          |A k8s DaemonSet|
|DaemonSet          |A k8s DaemonSet|
|Deployment         |A k8s Deployment|
|HttpRequest        |A speial component that can sent http/https requests. It's not tunnelling into the clusters, so the destination addresses must be available from the machine running k8deployer. Can be used, for example, to join regions running in separate clusters into a global entity, or to add users or other objects to newly created apps as they become ready. Supports json payloads|
|Ingress            |A k8s Ingress. These are normally created automatically based on the arguments to Deployment, StateFulSet and DaemonSet components|
|Job                |A k8d Job - typically a job that runs once during the deployment, for example to initialize a component when that component is ready.|
|Namespace          |A k8s Namespace. Usually created and maintained by the command-line settings for namespaces.|
|PersitentVolume    |A k8s PersitentVolume. These are normally created automatically based on the arguments to Deployment, StateFulSet and DaemonSet components|
|Role               |A k8s Role|
|RoleBinding        |A k8s RoleBinding|
|Secret             |A k8s Secret|
|Secret             |A k8s Secret|
|Service            |A k8s Service. These are normally created automatically based on the arguments to Deployment, StateFulSet and DaemonSet components|
|ServiceAccount     |A k8s ServiceAccount|
|StatefulSet        |A k8s StatefulSet|

### App
Think of a folder, containing files. An **App** component is just a way to group a basket of related components. \
These components may all depend on some other component. In stead of declaring that dependency on each individual component, 
you can declare it on the App component, and all it's chindren will inherit that dependency. 

Other components can also depend on the **App** component, and wait for all it's components to enter ready state before they are deployed.

If you have more than one component to deploy, you need a top-level **App** component to provide a list (it's `children:`) where you can declare your components.

Example of **App** as a top level placeholder.

```yaml
name: My-webapp
kind: App
children:
  - name: nodejs
    Kind: Deployments
    args: ...
    depends: mysql
  - name: mysql
    Kind: StatefulSet
    args: ...
```


### Deployment
A Deployment specifies one or more stateless pods (think, container) to be run. You specify the number of instances with the `replicas` argument.
Once deployed, kubernetes allows you to increase or decrease this number at any time, independent of k8deployer. 


Properties

|name                   |Required |Purpose
|-----------------------|:-------:|----------------|
|name                   |yes      |Unique name in the deployment file (unless variants for that name is used).|
|kind                   |yes      |typename of the component.|
|args                   |yes      |A key/value list of arguments where the name has a special meaning depending on the component `kind`. See the table below for the args recognized by **Deployment**.|
|defaultArgs            |no       |Like `args`, but also applied recursively to all child-components. 
|podSecurityContext     |no       |K8s **PodSecurityContext** object.|
|startupProbe           |no       |K8s **Probe** object.|
|livenessProbe          |no       |K8s **Probe** object.|
|readinessProbe         |no       |K8s **Probe** object.|
|depends                |no       |List of names of component this component depend on. This component will not be deployed until all the components in the list is ready (as determined by k8s).|
|variant                |no       |Variant-name for this component. See section about variants.|
|enabled                |no       |Boolean flag to allow some components to be disabled by default. They can be enabled by the `--enable=component-name` command-line argument.|
|labels                 |no       |k8s labels for the component. K8deployer will define `app` (used for selecting the component by services) and a number of labels starting with `k8dep-`. You are free to add your own labels.|


Arguments from `args`
|name                   |Required |Purpose
|-----------------------|:-------:|----------------|
|replicas               |no       |Number of replicas (instances).|
|imagePullSecrets.fromDockerLogin|no|Provide credentials to pull the container image. Require one argument; the path to a json file created by `docker login`. Typically `~/.docker/config.json`)|
|tlsSecret              |no       |Provide a k8s TLS secret for the contaier. The secret get's mounted as volume `/certs` in the pod. Takes two arguments: `key=path-to=keyfile` and `crt=path-to-certchain-file`.|
|port                   |no       |One or more ports that the pod exposes. See below.|
|service.type           |no       |If a port is exposed, a **Service** is normally created automatically. This argument allows you to specify it's type. Default is **ClusterIp**.|
|service.nodePort       |no       |Specify the NodePort for the pod's service (normally in the range 30000-32767). If you specify `service.nodePort` and not `service.type`, the service type is set to **NodePort**.|
|ingress.paths          |no       |Specify ingress paths to the pod. See below.|
|config.fromFile        |no       |Copies one or more config-file(s) to a volume `/config` in the path (via an automatically created `ConfigMap` component). Takes one argument: The path to the config-files, seperated by comma.|
|image                  |yes      |Container image (and tag) to use for the main container in the pod.|
|serviceAccountName     |no       |Name of a k8s **ServiceAccount** to associate with the pod.|
|pod.args               |no       |Command-line arguments for the pod. If no command is specified elsewhere, also the command. The command-line arguments are separated by space|
|pod.env                |no       |Environment variables for the pod. Consists of `var=value` pairs, separated by space.|
|pod.command            |no       |Command to execute in the pod. Overrides any default copmmand in the image.|
|imagePullPolicy        |no       |Specifies the k8s imagePullPolicy. One of `Always`, `Never`, `IfNotPresent`. Defaults to `Always`|
|pod.limits.memory      |no       |Specifies the max amount of memory to use by the pod. If it consumes more, k8s will kill it. Example: `pod.limits.memory=4Gi`|
|pod.memory             |no       |Convenience; sets both the memory required and memory limit (unless they are set specifically).|
|pod.limits.cpu         |no       |Specifies the max CPU usage for the pod. Can be specified with decimals. One equals one (hyperthreading) CPU core. 0.5 means 50% of one CPU core.|
|pod.requests.memory    |no       |Specifies the minimum required memory for the pod. The pod will not start until k8s finds a node with at least this amount of unreserved memory.|
|pod.requests.cpu       |no       |Specifies the minimum required CPU capacity for the pod. The pod will not start until k8s finds a node with at least this amount of unreserved CPU.|
|pod.cpu                |no       |Convenience; sets both the required CPU capacity and the CPU limit (unless they are set specifically).|
|imagePullSecrets       |no       |Specifies an existing docker hub secret to use when pulling container images."
|tls.secret             |no       |Specifies an existing TLS secret to use. Just like `tlsSecret`, it's mounted in the volume as `/certs`|


**Ports argument**
TBD

**Ingress argument**
Require an ingress controller (ex. *Traefik*) to be available in your k8s cluster. 


### Secret

### HttpRequest
This components sends a http or https request from k8deployer to a url. It can optionally contain a json payload. 

Example:
```yaml
name: example-request
kind: HttpRequest
args:
  target: POST https://api.example.com/apis/example
  json: '{"say":"Hello"}'
  auth: user=guest passwd=VerySecret123
  retry.count: "10"
  retry.delay.seconds: "20"
  log.message: 'Saying Hello via json'
```

Arguments
|name                   |Required |Purpose
|-----------------------|:-------:|----------------|
|auth                   |no       |Authentication. See below.|
|json                   |no       |An optional json payload.|
|log.message            |no       |A log message (at INFO level) to print when the request is about to be sent.|
|retry.count            |no       |Retry count if the request fails (returns something else than a reply in the 200 range).|
|retry.delay.seconds    |no       |Seconds to wait between retries.|
|target                 |yes      |Where and how to send the request. A verb identifying the HTTP request type (GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD) followed by space and then a url.|

Authentication types:
- **HTTP BasicAuth**: user=username <sp> passwd=password

