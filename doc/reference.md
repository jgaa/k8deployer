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
|args                   |yes      |A key/value list of arguments where the name has a special meaning depending on the component `kind`. See the table below for the args recognized by **Deployment**.|
|defaultArgs            |no       |Like `args`, but also applied recursively to all child-components. 
|depends                |no       |Array of names of component this component depend on. This component will not be deployed until all the components in the list is ready (as determined by k8s).|
|enabled                |no       |Boolean flag to allow some components to be disabled by default. They can be enabled by the `--enable=component-name` command-line argument.|
|kind                   |yes      |typename of the component.|
|labels                 |no       |k8s labels for the component. K8deployer will define `app` (used for selecting the component by services) and a number of labels starting with `k8dep-`. You are free to add your own labels.|
|livenessProbe          |no       |K8s **Probe** object.|
|name                   |yes      |Unique name in the deployment file (unless variants for that name is used).|
|podSecurityContext     |no       |K8s **PodSecurityContext** object.|
|readinessProbe         |no       |K8s **Probe** object.|
|startupProbe           |no       |K8s **Probe** object.|
|variant                |no       |Variant-name for this component. See section about variants.|


Arguments from `args`
|name                   |Required |Purpose
|-----------------------|:-------:|----------------|
|config.fromFile        |no       |Copies one or more config-file(s) to a volume `/config` in the path (via an automatically created `ConfigMap` component). Takes one argument: The path to the config-files, seperated by comma.|
|image                  |yes      |Container image (and tag) to use for the main container in the pod.|
|imagePullPolicy        |no       |Specifies the k8s imagePullPolicy. One of `Always`, `Never`, `IfNotPresent`. Defaults to `Always`|
|imagePullSecrets       |no       |Specifies an existing docker hub secret to use when pulling container images."
|imagePullSecrets.fromDockerLogin|no|Provide credentials to pull the container image. Require one argument; the path to a json file created by `docker login`. (Typically `~/.docker/config.json`)|
|ingress.paths          |no       |Specify ingress paths to the pod. See below.|
|ingress.annotations    |no       |Annotations for the ingress controller. Consists of `var=value` pairs, separated by space.|
|pod.args               |no       |Command-line arguments for the pod. If no command is specified elsewhere, also the command. The command-line arguments are separated by space|
|pod.command            |no       |Command to execute in the pod. Overrides any default command in the image.|
|pod.cpu                |no       |Convenience; sets both the required CPU capacity and the CPU limit (unless they are set specifically).|
|pod.env                |no       |Environment variables for the pod. Consists of `var=value` pairs, separated by space.|
|pod.limits.cpu         |no       |Specifies the max CPU usage for the pod. Can be specified with decimals. One equals one (hyper-threading) CPU core. 0.5 means 50% of one CPU core.|
|pod.limits.memory      |no       |Specifies the max amount of memory to use by the pod. If it consumes more, k8s will kill it. Example: `pod.limits.memory=4Gi`|
|pod.memory             |no       |Convenience; sets both the memory required and memory limit (unless they are set specifically).|
|pod.requests.cpu       |no       |Specifies the minimum required CPU capacity for the pod. The pod will not start until k8s finds a node with at least this amount of unreserved CPU.|
|pod.requests.memory    |no       |Specifies the minimum required memory for the pod. The pod will not start until k8s finds a node with at least this amount of unreserved memory.|
|port                   |no       |One or more ports that the pod exposes. See below.|
|replicas               |no       |Number of replicas (instances).|
|service.nodePort       |no       |Specify the NodePort for the pod's service (normally in the range 30000-32767). If you specify `service.nodePort` and not `service.type`, the service type is set to **NodePort**.|
|service.type           |no       |If a port is exposed, a **Service** is normally created automatically. This argument allows you to specify it's type. Default is **ClusterIp**.|
|serviceAccountName     |no       |Name of a k8s **ServiceAccount** to associate with the pod.|
|tls.secret             |no       |Specifies an existing TLS secret to use. Just like `tlsSecret`, it's mounted in the volume as `/certs`|
|tlsSecret              |no       |Provide a k8s TLS secret for the container. The secret gets mounted as volume `/certs` in the pod. Takes two arguments: `key=path-to=keyfile` and `crt=path-to-certchain-file`.|
|pod.scc.add            |no       |Provide one or a space-separated list of capabilities to add to the pod's security context. For example `SYS_PTRACE`|


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

Arguments from `args`
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

## Macros

The input yaml file is parsed for macros before it is serialized to internal objects. A macro is a variable, that may have a default value. At the time of the macro expansion, there is no context, except that the resulting text must be valid *yaml* format that can be transformed to json, and adhere to the type requirements of the objects it will serialize into. For example:

```yaml
name: ${example.name,123}
```

This is valid *yaml* that can be transformed to json: `"name":"${example.name,123}"`. Obviously, `name` will be serialized to a string, so all is well in the original data.

So, lets look at how this expression will be transformed by macro expansion. `${example.name,123}` means that we have a variable, *example.name* with a default value of *123*.
If we run k8deployer with the command-line argument `-v example.name=Marvin`, the expression will transform to `name: Marvin`, which is valid *yaml*. 
Then it will be transformed json: `"name":"Marvin"` which is valid json. Fially it will be serialized to a string that will hold the value. Since `"Marvin"` in json is a string, all is well.

However, if we *don't* specify anything on the command line, the expression will transform to `name: 123`, which is valid *yaml*. That will transform to json: `"name":123` which is valid json. But when we try to serialize that to a string, that will not work as expected. So, if you use a number as a default value to a variable that will be serialized to a string, you must put quotes around it, like below:

```yaml
name: ${example.name,"123"}
```

So, macros are variables with an optional default value. The names should contain a-z, numbers (but not be a number), dashes and point.

- ${name123} is valid
- ${name.123} is valid
- ${n} is valid
- ${123} is not valid
- ${$testme} is not valid (contains `$`)
- ${my-verylong_example.name} is valid

If the name is followed by a comma, what follows is a default value. Brackets are counted, so `${name,{{{test}}}}` is valid, where the default value expands to `{{{test}}}`
The default value can itself be a variable with it's own default value. 

```yaml
name: ${recursive.example,${alternative.var,"Marvin"}}
```

There are a few built in variables (see the section below), but in most cases you will use your own variable names, and specify their
value on the command line with the `-v` command line argument. For example:
```sh
k8deployer -v my.var=something -v my.other=something-else
```

As you understand, you can specify as many variables as you need, with pretty much whatever names you prefer.

The variables you specify on the command-line with the `-v` arguments have a global scope. They are available with the same value in 
all the clusters you deploy on in parallel.

You can also specify cluster-scoped variables.

```sh
k8deployer -v my.var=something -v my.other=something-else 
  ~/k8s/cluster1.conf:location=London,max.something=100 \
  ~/k8s/cluster2.conf:location=Berlin,max.something=123 \
  ~/k8s/cluster3.conf:location=NewYork,max.something=111,name=ny

```

In this example, we specified 2 global variables, and two cluster-scoped variables, `location` and `max.something`
for three clusters. ~/k8s/cluster*.conf are kubeconfig files for the three clusters. 

If we have a matrix of clusters and variables used during the deployments of these
three clusters, it can look like:

|Cluster   |Variable    |Value   |
|----------|------------|--------|
|cluster1  |my.var      |something|
|          |my.other    |something-else|
|          |location    |London|
|          |max.something|100|
|          |name        |cluster1|
|          |clusterId   |0 |
|cluster2  |my.var      |something|
|          |my.other    |something-else|
|          |location    |Berlin|
|          |max.something|123|
|          |name        |cluster2|
|          |clusterId   |1 |
|cluster3  |my.var      |something|
|          |my.other    |something-else|
|          |location    |NewYork|
|          |max.something|111|
|          |name        |ny|
|          |clusterId   |2 |

Note that the built in variable `name` assumes the name of the first part of the kubeconfig-file, unless you override it.
The built in `clusterId` increments for each cluster.

If you run k8deployer with output at debug level `-l debug`, it will print all the variables for all the clusters when 
it starts up.

### Functions

The macro expansion can also evaluate a few mathematical functions.

- **$eval(expr)**: Boolean expression that returns `true` or `false`.
- **$intexpr(expr)**: Expression returning an integer
- **$expr(expr)**: Expression returning a floating point value.

The functions forward the expression (as text) to [ExprTk](http://www.partow.net/programming/exprtk/), and cast the result to bool (*eval*) or int (*intexptr*).
This gives you the opportunity to do real math on the input data, with lots of flexibility as a result.

Below are a few examples from some of my own use-cases:

```yaml
name: example
kind: Deployment

# Disable on the first cluster
enabled: $eval(${clusterId} > 0)
```

If you deploy to multiple clusters in parallel, each cluster will get it's own cluster-scoped variable, `clusterId`, which is an integer. The first cluster you specify on the command-line will get value 0, then 1, 2 ,3 ...
The expression above makes sure that the Deployment will be disabled on the first cluster, and enabled on all the others. 


```yaml
name: example
kind: StatefulSet
args:
  pod.args:
    --cluster.system-replication-factor ${cluster.system.replication.factor,$intexpr(min(2,${db.replicas,1}))}
  replicas: "${db.replicas,1}"
```

The example above is used to set the system replication factor in a database with data-replication for resilience, and the number of replicas (instances).

Normally I want 2 replicas for any data object stored in the database. But if I deploy only 1 instance, that wont work, as it needs at least 2 instances to 
replicate any data to 2 instances. 

The variable `db.replicas` specify the number of database instances I am going to deploy. The variable `cluster.system.replication.factor` can override the value to `--cluster.system-replication-factor`.
However, normally I will only specify `-v db.replicas=3` (or some other number) on the command-line, and let the expression `$intexpr(min(2,${db.replicas,1}))` set the
number of copies of the data, based on `db.replicas`. If `db.replicas` is 1, the expression will return 1. Else, if `db.replicas` > 1, it will return 2.

You may notice that the default value `1` in `${db.replicas,1}` is repeated twice in the example above. 
The reason is that this expression don't declares the variable; it *refers* to it, and it provides a default value in case the variable is unset. 
There is currently no way to declare a variable in the yaml file. 


### Built-in variables

|Name           |Scope        | Purpose  | Default value|
|---------------|-------------|----------|--------------|



