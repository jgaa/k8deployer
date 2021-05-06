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
  log.message: 'Saying Hello vi json'
```

Target request types: GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD

Arguments
|name                   |Required |Purpose
|-----------------------|:-------:|----------------|
|auth                   |no       |Authentication. See below.|
|json                   |no       |An optional json payload.|
|log.message            |no       |A log message (at INFO level) to print when the request is about to be sent.|
|retry.count            |no       |Retry count if the request fails (returns something else than a reply in the 200 range).|
|retry.delay.seconds    |no       |Seconds to wait between retries.|
|target                 |yes      |Where and how to send the request. A verb identifying the HTTP request type (GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD) followed by a url.|

Authentication types:
- **HTTP BasicAuth**: user=username <sp> passwd=password

