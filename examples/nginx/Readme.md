# Examples deploying nginx

For simplicity, we will use minikube for these examples. 

## Simple example, using NodePort

To get started, install minikube and start it, for example with the command below:

```sh
minikube start --container-runtime='docker'

```

Then, assuming you are in the root of the k8deployer directoirfy,
deploy a very simple nginx setup, where we use a `NodePort` service
to reach the 3 instances of nginx.

The `nginx/nginx-deployment.yaml` file imports two files to a ConfigMap, and 
maps the ConfigMap as a volume to the pods. This gives nginx access to a config file
and a `index.html` file which it will show to users asking for the `/` path.

```yaml
name: nginx
kind: Deployment
args:
    replicas: "3"
    image: nginx
    port: "8080"
    service.type: NodePort
    config.fromFile: examples/nginx/nginx.conf,examples/nginx/index.html
    pod.command: nginx -c /config/nginx.conf
```

That's the whole declaration to deploy nginx.

The command below deploys the file. It deploys it in a kubernetes namespace `example`, which it will create if it don't exist.

```sh
~/src/k8deployer$ k8deployer -d examples/nginx/nginx-deployment.yaml --manage-namespace true -n example -k ~/.kube/config
```

Example Output:
```
2022-01-16 12:58:05.943 EET INFO 139879931520896 k8deployer 0.0.23. Log level: info
2022-01-16 12:58:05.945 EET INFO 139879931520896 config Preparing ...
2022-01-16 12:58:06.035 EET INFO 139879931520896 config Will connect directly to: https://192.168.49.2:8443
2022-01-16 12:58:06.123 EET INFO 139879931520896 config Deploying ...
2022-01-16 12:58:08.154 EET INFO 139879922636544 config/Namespace/example-ns Done in 2.03069 seconds
2022-01-16 12:58:08.159 EET INFO 139879922636544 config/ConfigMap/nginx-conf Done in 0.00516 seconds
2022-01-16 12:58:18.218 EET INFO 139879922636544 config/Service/nginx-svc Done in 2.02437 seconds
2022-01-16 12:58:18.218 EET INFO 139879922636544 config/Deployment/nginx Done in 10.05900 seconds
```

To connect to the service from the host machine:

```sh
curl $(minikube service -n example --url nginx-svc) -v
```

Output

```
* Expire in 0 ms for 6 (transfer 0x562e64c66fb0)
*   Trying 192.168.49.2...
* TCP_NODELAY set
* Expire in 200 ms for 4 (transfer 0x562e64c66fb0)
* Connected to 192.168.49.2 (192.168.49.2) port 31380 (#0)
> GET / HTTP/1.1
> Host: 192.168.49.2:31380
> User-Agent: curl/7.64.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< Server: nginx/1.21.5
< Date: Sun, 16 Jan 2022 18:23:08 GMT
< Content-Type: text/html
< Content-Length: 51
< Last-Modified: Sun, 16 Jan 2022 10:58:08 GMT
< Connection: keep-alive
< ETag: "61e3fa40-33"
< Accept-Ranges: bytes
< 
<html>
<body>
This is a test only.
</body>
</html>
* Connection #0 to host 192.168.49.2 left intact
```

This is the components in the namespace:

```
~/src/k8deployer$ kubectl -n example get all,cm,secret,ing 
NAME                         READY   STATUS    RESTARTS   AGE
pod/nginx-64c754dc5f-2tb7p   1/1     Running   0          7h34m
pod/nginx-64c754dc5f-btklz   1/1     Running   0          7h34m
pod/nginx-64c754dc5f-zqdzl   1/1     Running   0          7h34m

NAME                TYPE       CLUSTER-IP     EXTERNAL-IP   PORT(S)          AGE
service/nginx-svc   NodePort   10.105.43.98   <none>        8080:31380/TCP   7h34m

NAME                    READY   UP-TO-DATE   AVAILABLE   AGE
deployment.apps/nginx   3/3     3            3           7h34m

NAME                               DESIRED   CURRENT   READY   AGE
replicaset.apps/nginx-64c754dc5f   3         3         3       7h34m

NAME                         DATA   AGE
configmap/kube-root-ca.crt   1      7h34m
configmap/nginx-conf         2      7h34m

NAME                         TYPE                                  DATA   AGE
secret/default-token-j4hd4   kubernetes.io/service-account-token   3      7h34m
```
