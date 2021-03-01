#pragma once

/*! A subset of the kubernetes API that we use, or plan to use, in the near future
 *
 * https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.20/
 */

#include <string>
#include <vector>
#include <map>
#include <optional>

#include <boost/fusion/adapted.hpp>

namespace k8deployer::k8api {

using key_values_t = std::map<std::string, std::string>;
using labels_t = key_values_t;
using string_list_t = std::vector<std::string>;

struct Selector {
    labels_t matchLabels;
};

struct KeyValue {
    std::string name;
    std::string value;
};
using env_vars_t = std::vector<KeyValue>;

struct OwnerReference {
    std::string apiVersion;
    bool blockOwnerDeletion = false;
    bool controller = false;
    std::string kind;
    std::string uid;
};

using owner_references_t = std::vector<OwnerReference>;

struct ObjectMeta {
    std::string name;
    std::string namespace_;
    key_values_t annotations;
    labels_t labels;
    std::string clusterName;
    string_list_t finalizers;
    std::string generateName;
    int generation = 0;
    owner_references_t ownerReferences;
    std::string selfLink;
    std::string uid;
};

struct Secret {
    std::string apiVersion = "v1";
    std::string kind = "Secret";
    key_values_t data; // base64 encoded
    bool immutable = false;
    ObjectMeta metadata;
    key_values_t stringData;
    std::string type;
};

struct ObjectReference {
    std::string apiVersion;
    std::string fieldPath;
    std::string kind;
    std::string name;
    std::string namespace_;
    std::string resourceVersion;
    std::string uid;
};

struct Event {
    std::string action;
    std::string apiVersion;
    size_t count = 0;
    //MicroTime eventTime;
    std::string firstTimestamp;
    std::string lastTimestamp;
    std::string kind;
    std::string message;
    std::string reason;
    std::string type;
    std::string reportingComponent;
    std::string reportingInstance;
    ObjectMeta metadata;
    ObjectReference involvedObject;
    ObjectReference related;
};

using events_t = std::vector<Event>;

struct ListMeta {
    std::string continue_;
    int remainingItemCount = 0;
    std::string resourceVersion;
    std::string selfLink;
};

struct EventList {
    std::string apiVersion = "v1";
    events_t items;
    std::string kind;
    ListMeta metadata;
};

struct ContainerPort {
    std::string name;
    unsigned int containerPort = 0;
    std::string hostIP;
    unsigned int hostPort =0;
    std::string protocol;
};
using
container_ports_t = std::vector<ContainerPort>;


struct VolumeMount {
    std::string mountPath;
    std::string mountPropagation;
    std::string name;
    bool readOnly = false;
    std::string subPath;
    std::string subPathExpr;
};

using volume_mounts_t = std::vector<VolumeMount>;

using env_vars_t = std::vector<KeyValue>;

struct HTTPGetAction {
    using http_headers_t = std::vector<KeyValue>;
    std::string host;
    http_headers_t httpHeaders;
    std::string path;
    int port = 0;
    std::string scheme;
};

struct ExecAction {
    string_list_t command;
};

struct Time {
   std::string effect;
   std::string key;
   std::string operator_;
   std::optional<int> tolerationSeconds;
   std::string value;
};

struct TCPSocketAction {
    std::string effect;
    std::string key;
    std::string timeAdded;
    std::string value;
};

struct Probe {
    std::optional<ExecAction> exec;
    std::optional<HTTPGetAction> httpGet;
    std::optional<int> initialDelaySeconds;
    std::optional<int> periodSeconds;
    std::optional<int> successThreshold;
    std::optional<TCPSocketAction> tcpSocket;
    std::optional<int> timeoutSeconds;
};

struct SELinuxOptions {
    std::string level;
    std::string role;
    std::string type;
    std::string user;
};

struct SeccompProfile {
    std::string localhostProfile;
    std::string type;
};

struct Sysctl {
    std::string name;
    std::string value;
};

struct WindowsSecurityContextOptions {
    std::string gmsaCredentialSpec;
    std::string gmsaCredentialSpecName;
    std::string runAsUserName;
};

struct Capabilities {
    string_list_t add;
    string_list_t drop;
};

struct SecurityContext {
    bool allowPrivilegeEscalation = false;
    std::optional<Capabilities> capabilities;
    bool privileged = false;
    std::string procMount;
    bool readOnlyRootFilesystem = false;
    std::optional<int> runAsGroup;
    bool runAsNonRoot = false;
    std::optional<int> runAsUser;
    std::optional<SELinuxOptions> seLinuxOptions;
    std::optional<SeccompProfile> seccompProfile;
    std::optional<WindowsSecurityContextOptions> windowsOptions;
};

struct ResourceRequirements {
    key_values_t limits;
    key_values_t requests;
};

struct Container {
    std::string name;
    string_list_t args;
    string_list_t command; // overrides Pod's ENTRYPOINT
    env_vars_t env;
    std::string image;
    std::string imagePullPolicy;
    container_ports_t ports;
    volume_mounts_t volumeMounts;
    std::optional<Probe> startupProbe;
    std::optional<Probe> livenessProbe;
    std::optional<Probe> readinessProbe;
    std::optional<SecurityContext> securityContext;
    std::optional<ResourceRequirements> resources;
    bool stdin = false;
    bool stdinOnce = false;
    std::string terminationMessagePath;
    std::string terminationMessagePolicy;
    bool tty = false;
    std::string workingDir;
};

using containers_t = std::vector<Container>;

struct RollingUpdateDeployment {
    std::string maxSurge;
    std::string maxUnavailable;
};

struct DeploymentStrategy {
    std::string type;
    RollingUpdateDeployment rollingUpdate;
};

struct NodeSelectorRequirement {
    std::string key;
    std::string operator_; // NB!
    string_list_t values;
};

using node_selector_requirements_t = std::vector<NodeSelectorRequirement>;

struct NodeSelectorTerm {
    node_selector_requirements_t matchExpressions;
    node_selector_requirements_t matchFields;
};

using node_selector_terms_t = std::vector<NodeSelectorTerm>;

struct PreferredSchedulingTerm {
    NodeSelectorTerm preference;
    int weight = 0;
};

using preffered_scheduling_terms_t = std::vector<PreferredSchedulingTerm>;

struct NodeSelector {
    node_selector_terms_t nodeSelectorTerms;
};

struct NodeAffinity {
    preffered_scheduling_terms_t preferredDuringSchedulingIgnoredDuringExecution;
    std::optional<NodeSelector> requiredDuringSchedulingIgnoredDuringExecution;
};

struct LabelSelectorRequirement {
    std::string key;
    std::string operator_;
    string_list_t values;
};

using label_selector_requirements_t = std::vector<LabelSelectorRequirement>;

struct LabelSelector {
    label_selector_requirements_t matchExpressions;
    key_values_t matchLabels;
};

struct PodAffinityTerm {
    LabelSelector labelSelector;
    string_list_t namespaces;
    std::string topologyKey;
};
using pod_affinity_terms_t = std::vector<PodAffinityTerm>;

struct WeightedPodAffinityTerm {
    PodAffinityTerm podAffinityTerm;
    int weight = 0;
};

using weighted_pod_affinity_terms_t = std::vector<WeightedPodAffinityTerm>;

struct PodAffinity {
    weighted_pod_affinity_terms_t preferredDuringSchedulingIgnoredDuringExecution;
    pod_affinity_terms_t requiredDuringSchedulingIgnoredDuringExecution;
};

struct PodAntiAffinity {
    weighted_pod_affinity_terms_t preferredDuringSchedulingIgnoredDuringExecution;
    pod_affinity_terms_t requiredDuringSchedulingIgnoredDuringExecution;
};

struct Affinity {
    NodeAffinity nodeAffinity;
    PodAffinity podAffinity;
    PodAntiAffinity podAntiAffinity;
};

struct LocalObjectReference {
    std::string name;
};

using local_object_references_t = std::vector<LocalObjectReference>;

struct TopologySpreadConstraint {
    LabelSelector labelSelector;
    int maxSkew = 1;
    std::string topologyKey;
    std::string whenUnsatisfiable;
};

using tolology_spread_constraints_t = std::vector<TopologySpreadConstraint>;

struct HostPathVolumeSource {
    std::string path;
    std::string type;
};

struct LocalVolumeSource {
    std::string fsType;
    std::string path;
};

struct NFSVolumeSource {
    std::string path;
    bool readOnly = false;
    std::string server;
};

struct VolumeNodeAffinity {
    NodeSelector required;
};

struct PersistentVolumeSpec {
    string_list_t accessModes;
    std::optional<HostPathVolumeSource> hostPath;
    std::optional<LocalVolumeSource> local;
    std::optional<NFSVolumeSource> nfs;
    string_list_t mountOptions;
    std::optional<VolumeNodeAffinity> nodeAffinity;
    std::string persistentVolumeReclaimPolicy;
    std::string storageClassName;
    std::string volumeMode;
    key_values_t capacity;
    key_values_t claimRef;
};

struct PersistentVolumeClaimVolumeSource {
    std::string claimName;
    bool readOnly = false;
};

struct VolumeAttachmentSource {
    PersistentVolumeSpec inlineVolumeSpec;
    std::string persistentVolumeName;
};

struct VolumeAttachmentSpec {
    std::string attacher;
    std::string nodeName;
    VolumeAttachmentSource source;
};

struct KeyToPath {
    std::string key;
    std::optional<int> mode;
    std::string path;
};

using key_to_paths_t = std::vector<KeyToPath>;

struct ConfigMapVolumeSource {
    int defaultMode = 0644;
    key_to_paths_t items;
    std::string name;
    bool optional = false;
};

struct Quantity {
    size_t handSize = 8;
    size_t queueLengthLimit = 50;
    size_t queues = 64;
};

struct EmptyDirVolumeSource {
    std::string medium;
    Quantity sizeLimit;
};

struct SecretVolumeSource {
    std::optional<int> defaultMode;
    std::vector<KeyToPath> items;
    std::optional<bool> optional;
    std::string secretName;
};

struct Volume {
    std::string name;
    std::optional<ConfigMapVolumeSource> configMap;
    std::optional<EmptyDirVolumeSource> emptyDir;
    std::optional<HostPathVolumeSource> hostPath;
    std::optional<NFSVolumeSource> nfs;
    std::optional<PersistentVolumeClaimVolumeSource> persistentVolumeClaim;
    std::optional<SecretVolumeSource> secret;
};

using volumes_t = std::vector<Volume>;

struct PersistentVolumeStatus {
    std::string message;
    std::string phase;
    std::string reason;
};

struct PersistentVolume {
    std::string apiVersion = "v1";
    std::string kind = "PersistentVolume";
    ObjectMeta metadata;
    PersistentVolumeSpec spec;
    std::optional<PersistentVolumeStatus> status;
};

struct HostAlias {
    string_list_t hostnames;
    std::string ip;
};

struct PodSecurityContext {
    std::optional<int> fsGroup;
    std::string fsGroupChangePolicy;
    std::optional<int> runAsGroup;
    std::optional<bool> runAsNonRoot;
    std::optional<int> runAsUser;
    std::optional<SELinuxOptions> seLinuxOptions;
    std::optional<SeccompProfile> seccompProfile;
    std::vector<int> supplementalGroups;
    std::vector<Sysctl> sysctls;
    std::optional<WindowsSecurityContextOptions> windowsOptions;
};

struct Toleration {
    std::string effect;
    std::string key;
    std::string operator_;
    std::optional<int> tolerationSeconds;
    std::string value;
};

struct PodReadinessGate {
   std::string conditionType;
};

struct PodDNSConfigOption {
   std::string name;
   std::string value;
};

struct PodDNSConfig {
    string_list_t nameservers;
    std::vector<PodDNSConfigOption> options;
    string_list_t searches;
};

struct PodSpec {
    size_t activeDeadlineSeconds = 0;
    std::optional<Affinity> affinity;
    std::optional<bool> automountServiceAccountToken;
    containers_t containers;
    std::optional<PodDNSConfig> dnsConfig;
    std::string dnsPolicy;
    std::optional<bool> enableServiceLinks;
    // std::vector<EphemeralContainer> ephemeralContainers
    std::vector<HostAlias> hostAliases;
    std::optional<bool> hostIPC;
    std::optional<bool> hostNetwork;
    std::optional<bool> hostPID;
    std::string hostname;
    local_object_references_t imagePullSecrets;
    containers_t initContainers;
    std::string nodeName;
    key_values_t nodeSelector;
    std::string preemptionPolicy;
    std::optional<int> priority;
    std::string priorityClassName;
    std::vector<PodReadinessGate> readinessGates;
    std::string runtimeClassName;
    std::string schedulerName;
    std::optional<PodSecurityContext> securityContext;
    std::string serviceAccountName;
    std::optional<bool> setHostnameAsFQDN;
    std::optional<bool> shareProcessNamespace;
    std::string subdomain;
    std::vector<Toleration> tolerations;
    std::string restartPolicy;
    tolology_spread_constraints_t topologySpreadConstraints;
    volumes_t volumes;
};

struct PodTemplateSpec {
    ObjectMeta metadata;
    PodSpec spec;
};

struct JobSpec {
    size_t activeDeadlineSeconds = 0;
    size_t backoffLimit;
    std::optional<bool> manualSelector;
    size_t parallelism = 0;
    LabelSelector selector;
    PodTemplateSpec template_;
};

struct JobCondition {
    std::string lastProbeTime;
    std::string lastTransitionTime;
    std::string message;
    std::string reason;
    std::string status;
    std::string type;
};

struct JobStatus {
    size_t active = 0;
    std::string completionTime;
    std::vector<JobCondition> conditions;
    size_t failed = 0;
    std::string startTime;
    size_t succeeded;
};

struct Job {
    std::string apiVersion = "batch/v1";
    std::string kind = "Job";
    ObjectMeta metadata;
    JobSpec spec;
    size_t ttlSecondsAfterFinished = 0; // NB: Not always available...
    std::optional<JobStatus> status;
};

struct DeploymentSpec {
    std::optional<size_t> replicas;
    LabelSelector selector;
    DeploymentStrategy strategy;
    PodTemplateSpec template_;
};

struct DeploymentCondition {
    std::string lastTransitionTime;
    std::string lastUpdateTime;
    std::string message;
    std::string reason;
    std::string status;
    std::string type;
};

struct DeploymentStatus {
    size_t availableReplicas = 0;
    size_t collisionCount = 0;
    std::vector<DeploymentCondition> conditions;
    size_t observedGeneration = 0;
    size_t readyReplicas = 0;
    size_t replicas = 0;
    size_t unavailableReplicas = 0;
    size_t updatedReplicas = 0;
};

struct Deployment {
    std::string apiVersion = "apps/v1";
    std::string kind = "Deployment";
    ObjectMeta metadata;
    DeploymentSpec spec;
    std::optional<DeploymentStatus> status;
};

struct ServicePort {
    std::string appProtocol;
    std::string name;
    int nodePort = 0;
    int port = 0;
    std::string protocol;
    std::string targetPort;
};

using service_ports_t = std::vector<ServicePort>;

struct ServiceSpec {
    std::string clusterIP;
    string_list_t clusterIPs;
    string_list_t externalIPs;
    std::string externalName;
    std::string externalTrafficPolicy;
    string_list_t ipFamilies;
    std::string ipFamilyPolicy;
    string_list_t loadBalancerSourceRanges;
    service_ports_t ports;
    labels_t selector;
    //std::string sessionAffinity;
    string_list_t topologyKeys;
    std::string type;  // ExternalName, *ClusterIP, NodePort, and LoadBalancer
};


struct Condition {
    std::string lastTransitionTime;
    std::string message;
    size_t observedGeneration = 0;
    std::string reason;
    std::string status;
    std::string type;
};

struct PortStatus {
    std::string error;
    int port = 0;
    std::string protocol;
};

struct LoadBalancerIngress {
    std::string hostname;
    std::string ip;
    std::vector<PortStatus> ports;
};

struct LoadBalancerStatus {
    std::vector<LoadBalancerIngress> ingress;
};

struct ServiceStatus {
    std::vector<Condition> conditions;
    std::optional<LoadBalancerStatus> loadBalancer;
};

struct Service {
    std::string apiVersion = "v1";
    std::string kind = "Service";
    ObjectMeta metadata;
    ServiceSpec spec;
    std::optional<ServiceStatus> status;
};

struct ConfigMap {
    std::string apiVersion = "v1";
    std::string kind = "ConfigMap";
    ObjectMeta metadata;
    bool immutable = false;
    key_values_t data;
    key_values_t binaryData; // base64 encoded
};

struct RollingUpdateStatefulSetStrategy {
    size_t partition = 0;
};

struct StatefulSetUpdateStrategy {
    std::optional<RollingUpdateStatefulSetStrategy> rollingUpdate;
    std::string type;
};

struct TypedLocalObjectReference {
    std::string apiGroup;
    std::string kind;
    std::string name;
};

struct PersistentVolumeClaimCondition {
    std::string lastProbeTime;
    std::string lastTransitionTime;
    std::string message;
    std::string reason;
    std::string status;
    std::string type;
};

struct PersistentVolumeClaimStatus {
    string_list_t accessModes;
    key_values_t capacity;
    std::vector<PersistentVolumeClaimCondition> conditions;
    std::string phase;
};

struct PersistentVolumeClaimSpec {
    string_list_t accessModes;
    TypedLocalObjectReference dataSource;
    std::optional<ResourceRequirements> resources;
    LabelSelector selector;
    std::string storageClassName;
    std::string volumeMode;
    std::string volumeName;
    std::optional<PersistentVolumeClaimStatus> status;
};

struct PersistentVolumeClaim {
    std::string apiVersion = "v1";
    std::string kind = "PersistentVolumeClaim";
    ObjectMeta metadata;
    PersistentVolumeClaimSpec spec;
};

struct StatefulSetSpec {
    std::string podManagementPolicy;
    std::optional<size_t> replicas;
    std::optional<size_t> revisionHistoryLimit;
    std::optional<LabelSelector> selector;
    std::string serviceName;
    std::optional<PodTemplateSpec> template_;
    std::optional<StatefulSetUpdateStrategy> updateStrategy;
    std::vector<PersistentVolumeClaim> volumeClaimTemplates;
};

struct StatefulSetCondition {
    std::string lastTransitionTime;
    std::string gmessage;
    std::string reason;
    std::string status;
    std::string type;
};

struct StatefulSetStatus {
    size_t collisionCount = 0;
    std::vector<StatefulSetCondition> conditions;
    size_t currentReplicas = 0;
    std::string currentRevision;
    size_t observedGeneration = 0;
    size_t readyReplicas = 0;
    size_t replicas = 0;
    std::string updateRevision;
    size_t updatedReplicas = 0;
};

struct StatefulSet {
    std::string apiVersion = "apps/v1";
    std::string kind = "StatefulSet";
    std::optional<ObjectMeta> metadata;
    std::optional<StatefulSetSpec> spec;
    std::optional<StatefulSetStatus> status;
};

struct ServiceBackendPort {
    std::string name;
    std::optional<int> number;
};

struct IngressServiceBackend {
    std::string name;
    ServiceBackendPort port;
};

struct IngressBackend {
    std::optional<TypedLocalObjectReference> resource;

    // networking.k8s.io/v1
    std::optional<IngressServiceBackend> service;

    // networking.k8s.io/v1beta1
    std::string serviceName;
    std::string servicePort;

    void setServiceName(const std::string& version, const std::string& name) {
        if (version == betaName()) {
            serviceName = name;
        } else {
            if (!service)
                service.emplace();
            service->name = name;
        }
    }

    void setServicePortName(const std::string& version, const std::string& name) {
        if (version == betaName()) {
            servicePort = name;
        } else {
            if (!service)
                service.emplace();
            service->port.name = name;
        }
    }

    std::string getServiceName(const std::string& version) const {
        if (version == betaName())
            return serviceName;
        if (service)
            return service->name;
        return {};
    }

    std::string getServicePortName(const std::string& version) const {
        if (version == betaName())
            return servicePort;
        if (service)
            return service->port.name;
        return {};
    }

    static const std::string& betaName() noexcept {
        static const std::string name{"networking.k8s.io/v1beta1"};
        return name;
    }
};


struct HTTPIngressPath {
    std::optional<IngressBackend> backend;
    std::string path;
    std::string pathType;
};

struct HTTPIngressRuleValue {
    std::vector<HTTPIngressPath> paths;
};

struct IngressRule {
    std::string host;
    std::optional<HTTPIngressRuleValue> http;
};

struct IngressTLS {
    string_list_t hosts;
    std::string secretName;
};

struct IngressSpec {
    std::optional<IngressBackend> defaultBackend;
    std::string ingressClassName;
    std::vector<IngressRule> rules;
    std::vector<IngressTLS> tls;
};

struct IngressStatus {
    LoadBalancerStatus loadBalancer;
};

struct Ingress {
    std::string apiVersion = "networking.k8s.io/v1";
    std::string kind = "Ingress";
    ObjectMeta metadata;
    std::optional<IngressSpec> spec;
    std::optional<IngressStatus> status;
};

struct NamespaceSpec {
    string_list_t finalizers;
};

struct NamespaceCondition {
    std::string lastTransitionTime;
    std::string message;
    std::string reason;
    std::string status;
    std::string type;
};

struct NamespaceStatus {
    std::vector<NamespaceCondition> conditions;
    std::string phase;
};

struct Namespace {
    std::string apiVersion = "v1";
    std::string kind = "Namespace";
    ObjectMeta metadata;
    std::optional<NamespaceSpec> spec;
    std::optional<NamespaceStatus> status;
};

struct RollingUpdateDaemonSet {
    size_t maxUnavailable = 0;
};

struct DaemonSetUpdateStrategy {
    std::optional<RollingUpdateDaemonSet> rollingUpdate;
    std::string type;
};

struct DaemonSetSpec {
    size_t minReadySeconds = 0;
    size_t revisionHistoryLimit = 0;
    LabelSelector selector;
    PodTemplateSpec template_;
    std::optional<DaemonSetUpdateStrategy> updateStrategy;
};

struct DaemonSetCondition {
    std::string lastTransitionTime;
    std::string message;
    std::string reason;
    std::string status;
    std::string type;
};

struct DaemonSetStatus {
    size_t collisionCount = 0;
    std::vector<DaemonSetCondition> conditions;
    size_t currentNumberScheduled = 0;
    size_t desiredNumberScheduled = 0;
    size_t numberAvailable = 0;
    size_t numberMisscheduled = 0;
    size_t numberReady = 0;
    size_t numberUnavailable = 0;
    size_t observedGeneration = 0;
    size_t updatedNumberScheduled = 0;
};

struct DaemonSet {
    std::string apiVersion = "apps/v1";
    std::string kind = "DaemonSet";
    ObjectMeta metadata;
    std::optional<DaemonSetSpec> spec;
    std::optional<DaemonSetStatus> status;
};

struct PolicyRule {
    string_list_t apiGroups;
    string_list_t nonResourceURLs;
    string_list_t resourceNames;
    string_list_t resources;
    string_list_t verbs;
};

struct Role {
    std::string apiVersion = "rbac.authorization.k8s.io/v1";
    std::string kind = "Role";
    ObjectMeta metadata;
    std::vector<PolicyRule> rules;
};

struct RoleRef {
    std::string apiGroup;
    std::string kind;
    std::string name;
};

struct Subject {
    std::string apiGroup;
    std::string kind;
    std::string name;
    std::string namespace_;
};

struct RoleBinding {
    std::string apiVersion = "rbac.authorization.k8s.io/v1";
    std::string kind = "RoleBinding";
    ObjectMeta metadata;
    RoleRef roleRef;
    std::vector<Subject> subjects;
};

struct AggregationRule {
    std::vector<LabelSelector> clusterRoleSelectors;
};

struct ClusterRole {
    std::string apiVersion = "rbac.authorization.k8s.io/v1";
    std::string kind = "ClusterRole";
    ObjectMeta metadata;
    std::optional<AggregationRule> aggregationRule;
    std::vector<PolicyRule> rules;
};

struct ClusterRoleBinding {
    std::string apiVersion = "rbac.authorization.k8s.io/v1";
    std::string kind = "ClusterRoleBinding";
    ObjectMeta metadata;
    RoleRef roleRef;
    std::vector<Subject> subjects;
};

struct ServiceAccount {
    std::string apiVersion = "v1";
    std::string kind = "ServiceAccount";
    ObjectMeta metadata;
    std::optional<bool> automountServiceAccountToken;
    std::vector<LocalObjectReference> imagePullSecrets;
    std::vector<ObjectReference> secrets;
};

} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Selector,
    (k8deployer::k8api::labels_t, matchLabels)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::KeyValue,
    (std::string, name)
    (std::string, value)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ObjectMeta,
    (std::string, name)
    (std::string, namespace_)
    (k8deployer::k8api::key_values_t, annotations)
    (k8deployer::k8api::labels_t, labels)
    (std::string, clusterName)
    (k8deployer::k8api::string_list_t, finalizers)
    (std::string, generateName)
    (int, generation)
    (k8deployer::k8api::owner_references_t, ownerReferences)
    (std::string, selfLink)
    (std::string, uid)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ObjectReference,
    (std::string, apiVersion)
    (std::string, fieldPath)
    (std::string, kind)
    (std::string, name)
    (std::string, namespace_)
    (std::string, resourceVersion)
    (std::string, uid)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Event,
    (std::string, action)
    (std::string, apiVersion)
    (size_t, count)
    //MicroTime, eventTime)
    (std::string, firstTimestamp)
    (std::string, lastTimestamp)
    (std::string, kind)
    (std::string, message)
    (std::string, reason)
    (std::string, type)
    (std::string, reportingComponent)
    (std::string, reportingInstance)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::ObjectReference, involvedObject)
    (k8deployer::k8api::ObjectReference, related)
);


BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ListMeta,
(std::string, continue_) // NB
(int, remainingItemCount)
(std::string, resourceVersion)
(std::string, selfLink)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::EventList,
(std::string, apiVersion)
(k8deployer::k8api::events_t, items)
(std::string, kind)
(k8deployer::k8api::ListMeta, metadata)
);


BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ContainerPort,
    (std::string, name)
    (unsigned int, containerPort)
    (std::string, hostIP)
    (unsigned int, hostPort)
    (std::string, protocol)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::VolumeMount,
    (std::string, mountPath)
    (std::string, mountPropagation)
    (std::string, name)
    (bool, readOnly)
    (std::string, subPath)
    (std::string, subPathExpr)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Time,
   (std::string, effect)
   (std::string, key)
   (std::string, operator_)
   (std::optional<int>, tolerationSeconds)
   (std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::TCPSocketAction,
    (std::string, effect)
    (std::string, key)
    (std::string, timeAdded)
    (std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::HTTPGetAction,
    (std::string, host)
    (k8deployer::k8api::HTTPGetAction::http_headers_t, httpHeaders)
    (std::string, path)
    (int, port)
    (std::string, scheme)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ExecAction,
    (k8deployer::k8api::string_list_t, command)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Probe,
    (std::optional<k8deployer::k8api::ExecAction>, exec)
    (std::optional<k8deployer::k8api::HTTPGetAction>, httpGet)
    (std::optional<int>, initialDelaySeconds)
    (std::optional<int>, periodSeconds)
    (std::optional<int>, successThreshold)
    (std::optional<k8deployer::k8api::TCPSocketAction>, tcpSocket)
    (std::optional<int>, timeoutSeconds)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Capabilities,
    (k8deployer::k8api::string_list_t, add)
    (k8deployer::k8api::string_list_t, drop)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::SecurityContext,
    (bool, allowPrivilegeEscalation)
    (std::optional<k8deployer::k8api::Capabilities>, capabilities)
    (bool, privileged)
    (std::string, procMount)
    (bool, readOnlyRootFilesystem)
    (std::optional<int>, runAsGroup)
    (bool, runAsNonRoot)
    (std::optional<int>, runAsUser)
    (std::optional<k8deployer::k8api::SELinuxOptions>, seLinuxOptions)
    (std::optional<k8deployer::k8api::SeccompProfile>, seccompProfile)
    (std::optional<k8deployer::k8api::WindowsSecurityContextOptions>, windowsOptions)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ResourceRequirements,
    (k8deployer::k8api::key_values_t, limits)
    (k8deployer::k8api::key_values_t, requests)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Container,
    (std::string, name)
    (k8deployer::k8api::string_list_t, args)
    (k8deployer::k8api::string_list_t, command),
    (k8deployer::k8api::env_vars_t, env)
    (std::string, image)
    (std::string, imagePullPolicy)
    (k8deployer::k8api::container_ports_t, ports)
    (k8deployer::k8api::volume_mounts_t, volumeMounts)
    (std::optional<k8deployer::k8api::Probe>, startupProbe)
    (std::optional<k8deployer::k8api::Probe>, livenessProbe)
    (std::optional<k8deployer::k8api::Probe>, readinessProbe)
    (std::optional<k8deployer::k8api::SecurityContext>, securityContext)
    (std::optional<k8deployer::k8api::ResourceRequirements>, resources)
    (bool, stdin)
    (bool, stdinOnce)
    (std::string, terminationMessagePath)
    (std::string, terminationMessagePolicy)
    (bool, tty)
    (std::string, workingDir)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::RollingUpdateDeployment,
    (std::string, maxSurge)
    (std::string, maxUnavailable)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DeploymentStrategy,
    (std::string, type)
    (k8deployer::k8api::RollingUpdateDeployment, rollingUpdate)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NodeSelectorRequirement,
    (std::string, key)
    (std::string, operator_), //, NB!
    (k8deployer::k8api::string_list_t, values)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NodeSelectorTerm,
    (k8deployer::k8api::node_selector_requirements_t, matchExpressions)
    (k8deployer::k8api::node_selector_requirements_t, matchFields)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PreferredSchedulingTerm,
    (k8deployer::k8api::NodeSelectorTerm, preference)
    (int, weight)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NodeSelector,
    (k8deployer::k8api::node_selector_terms_t, nodeSelectorTerms)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NodeAffinity,
    (k8deployer::k8api::preffered_scheduling_terms_t, preferredDuringSchedulingIgnoredDuringExecution)
    (std::optional<k8deployer::k8api::NodeSelector>, requiredDuringSchedulingIgnoredDuringExecution)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::LabelSelectorRequirement,
    (std::string, key)
    (std::string, operator_)
    (k8deployer::k8api::string_list_t, values)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::LabelSelector,
    (k8deployer::k8api::label_selector_requirements_t, matchExpressions)
    (k8deployer::k8api::key_values_t, matchLabels)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodAffinityTerm,
    (k8deployer::k8api::LabelSelector, labelSelector)
    (k8deployer::k8api::string_list_t, namespaces)
    (std::string, topologyKey)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::WeightedPodAffinityTerm,
    (k8deployer::k8api::PodAffinityTerm, podAffinityTerm)
    (int, weight)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodAffinity,
    (k8deployer::k8api::weighted_pod_affinity_terms_t, preferredDuringSchedulingIgnoredDuringExecution)
    (k8deployer::k8api::pod_affinity_terms_t, requiredDuringSchedulingIgnoredDuringExecution)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodAntiAffinity,
    (k8deployer::k8api::weighted_pod_affinity_terms_t, preferredDuringSchedulingIgnoredDuringExecution)
    (k8deployer::k8api::pod_affinity_terms_t, requiredDuringSchedulingIgnoredDuringExecution)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Affinity,
    (k8deployer::k8api::NodeAffinity, nodeAffinity)
    (k8deployer::k8api::PodAffinity, podAffinity)
    (k8deployer::k8api::PodAntiAffinity, podAntiAffinity)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::LocalObjectReference,
    (std::string, name)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::TopologySpreadConstraint,
    (k8deployer::k8api::LabelSelector, labelSelector)
    (int, maxSkew)
    (std::string, topologyKey)
    (std::string, whenUnsatisfiable)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::HostPathVolumeSource,
    (std::string, path)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::LocalVolumeSource,
    (std::string, fsType)
    (std::string, path)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NFSVolumeSource,
    (std::string, path)
    (bool, readOnly)
    (std::string, server)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::VolumeNodeAffinity,
    (k8deployer::k8api::NodeSelector, required)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeSpec,
    (k8deployer::k8api::string_list_t, accessModes)
    (std::optional<k8deployer::k8api::HostPathVolumeSource>, hostPath)
    (std::optional<k8deployer::k8api::LocalVolumeSource>, local)
    (std::optional<k8deployer::k8api::NFSVolumeSource>, nfs)
    (k8deployer::k8api::string_list_t, mountOptions)
    (std::optional<k8deployer::k8api::VolumeNodeAffinity>, nodeAffinity)
    (std::string, persistentVolumeReclaimPolicy)
    (std::string, storageClassName)
    (std::string, volumeMode)
    (k8deployer::k8api::key_values_t, capacity)
    (k8deployer::k8api::key_values_t, claimRef)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeStatus,
    (std::string, message)
    (std::string, phase)
    (std::string, reason)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolume,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::PersistentVolumeSpec, spec)
    (std::optional<k8deployer::k8api::PersistentVolumeStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeClaimVolumeSource,
    (std::string, claimName)
    (bool, readOnly)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::VolumeAttachmentSource,
    (k8deployer::k8api::PersistentVolumeSpec, inlineVolumeSpec)
    (std::string, persistentVolumeName)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::VolumeAttachmentSpec,
    (std::string, attacher)
    (std::string, nodeName)
    (k8deployer::k8api::VolumeAttachmentSource, source)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::KeyToPath,
    (std::string, key)
    (std::optional<int>, mode)
    (std::string, path)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ConfigMapVolumeSource,
    (int, defaultMode)
    (k8deployer::k8api::key_to_paths_t, items)
    (std::string, name)
    (bool, optional)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Quantity,
    (size_t, handSize)
    (size_t, queueLengthLimit)
    (size_t, queues)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::EmptyDirVolumeSource,
    (std::string, medium)
    (k8deployer::k8api::Quantity, sizeLimit)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::SecretVolumeSource,
    (std::optional<int>, defaultMode)
    (std::vector<k8deployer::k8api::KeyToPath>, items)
    (std::optional<bool>, optional)
    (std::string, secretName)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Volume,
    (std::string, name)
    (std::optional<k8deployer::k8api::ConfigMapVolumeSource>, configMap)
    (std::optional<k8deployer::k8api::EmptyDirVolumeSource>, emptyDir)
    (std::optional<k8deployer::k8api::HostPathVolumeSource>, hostPath)
    (std::optional<k8deployer::k8api::NFSVolumeSource>, nfs)
    (std::optional<k8deployer::k8api::PersistentVolumeClaimVolumeSource>, persistentVolumeClaim)
    (std::optional<k8deployer::k8api::SecretVolumeSource>, secret)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::SELinuxOptions,
    (std::string, level)
    (std::string, role)
    (std::string, type)
    (std::string, user)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::SeccompProfile,
    (std::string, localhostProfile)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Sysctl,
    (std::string, name)
    (std::string, value)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::WindowsSecurityContextOptions,
    (std::string, gmsaCredentialSpec)
    (std::string, gmsaCredentialSpecName)
    (std::string, runAsUserName)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::HostAlias,
    (k8deployer::k8api::string_list_t, hostnames)
    (std::string, ip)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodSecurityContext,
    (std::optional<int>, fsGroup)
    (std::string, fsGroupChangePolicy)
    (std::optional<int>, runAsGroup)
    (std::optional<bool>, runAsNonRoot)
    (std::optional<int>, runAsUser)
    (std::optional<k8deployer::k8api::SELinuxOptions>, seLinuxOptions)
    (std::optional<k8deployer::k8api::SeccompProfile>, seccompProfile)
    (std::vector<int>, supplementalGroups)
    (std::vector<k8deployer::k8api::Sysctl>, sysctls)
    (std::optional<k8deployer::k8api::WindowsSecurityContextOptions>, windowsOptions)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Toleration,
    (std::string, effect)
    (std::string, key)
    (std::string, operator_)
    (std::optional<int>, tolerationSeconds)
    (std::string, value)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodReadinessGate,
    (std::string, conditionType)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodDNSConfigOption,
    (std::string, name)
    (std::string, value)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodDNSConfig,
    (k8deployer::k8api::string_list_t, nameservers)
    (std::vector<k8deployer::k8api::PodDNSConfigOption>, options)
    (k8deployer::k8api::string_list_t, searches)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodSpec,
    (size_t, activeDeadlineSeconds)
    (std::optional<k8deployer::k8api::Affinity>, affinity)
    (std::optional<bool>, automountServiceAccountToken)
    (k8deployer::k8api::containers_t, containers)
    (std::optional<k8deployer::k8api::PodDNSConfig>, dnsConfig)
    (std::string, dnsPolicy)
    (std::optional<bool>, enableServiceLinks)
    (std::vector<k8deployer::k8api::HostAlias>, hostAliases)
    (std::optional<bool>, hostIPC)
    (std::optional<bool>, hostNetwork)
    (std::optional<bool>, hostPID)
    (std::string, hostname)
    (k8deployer::k8api::local_object_references_t, imagePullSecrets)
    (k8deployer::k8api::containers_t, initContainers)
    (std::string, nodeName)
    (k8deployer::k8api::key_values_t, nodeSelector)
    (std::string, preemptionPolicy)
    (std::optional<int>, priority)
    (std::string, priorityClassName)
    (std::vector<k8deployer::k8api::PodReadinessGate>, readinessGates)
    (std::string, runtimeClassName)
    (std::string, schedulerName)
    (std::optional<k8deployer::k8api::PodSecurityContext>, securityContext)
    (std::string, serviceAccountName)
    (std::optional<bool>, setHostnameAsFQDN)
    (std::optional<bool>, shareProcessNamespace)
    (std::string, subdomain)
    (std::vector<k8deployer::k8api::Toleration>, tolerations)
    (std::string, restartPolicy)
    (k8deployer::k8api::tolology_spread_constraints_t, topologySpreadConstraints)
    (k8deployer::k8api::volumes_t, volumes)
);



BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodTemplateSpec,
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::PodSpec, spec)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::JobSpec,
    (size_t, activeDeadlineSeconds)
    (size_t, backoffLimit)
    (std::optional<bool>, manualSelector)
    (size_t, parallelism)
    (k8deployer::k8api::LabelSelector, selector)
    (k8deployer::k8api::PodTemplateSpec, template_)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::JobCondition,
    (std::string, lastProbeTime)
    (std::string, lastTransitionTime)
    (std::string, message)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::JobStatus,
    (size_t, active)
    (std::string, completionTime)
    (std::vector<k8deployer::k8api::JobCondition>, conditions)
    (size_t, failed)
    (std::string, startTime)
    (size_t, succeeded)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Job,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::JobSpec, spec)
    (size_t, ttlSecondsAfterFinished)
    (std::optional<k8deployer::k8api::JobStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DeploymentSpec,
    (std::optional<size_t>, replicas)
    (k8deployer::k8api::LabelSelector, selector)
    (k8deployer::k8api::DeploymentStrategy, strategy)
    (k8deployer::k8api::PodTemplateSpec, template_), //, NB:
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DeploymentCondition,
    (std::string, lastTransitionTime)
    (std::string, lastUpdateTime)
    (std::string, message)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DeploymentStatus,
    (size_t, availableReplicas)
    (size_t, collisionCount)
    (std::vector<k8deployer::k8api::DeploymentCondition>, conditions)
    (size_t, observedGeneration)
    (size_t, readyReplicas)
    (size_t, replicas)
    (size_t, unavailableReplicas)
    (size_t, updatedReplicas)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Deployment,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::DeploymentSpec, spec)
    (std::optional<k8deployer::k8api::DeploymentStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ServicePort,
    (std::string, appProtocol)
    (std::string, name)
    (int, nodePort)
    (int, port)
    (std::string, protocol)
    (std::string, targetPort)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ServiceSpec,
    (std::string, clusterIP)
    (k8deployer::k8api::string_list_t, clusterIPs)
    (k8deployer::k8api::string_list_t, externalIPs)
    (std::string, externalName)
    (std::string, externalTrafficPolicy)
    (k8deployer::k8api::string_list_t, ipFamilies)
    (std::string, ipFamilyPolicy)
    (k8deployer::k8api::string_list_t, loadBalancerSourceRanges)
    (k8deployer::k8api::service_ports_t, ports)
    (k8deployer::k8api::labels_t, selector)
    (k8deployer::k8api::string_list_t, topologyKeys)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Condition,
    (std::string, lastTransitionTime)
    (std::string, message)
    (size_t, observedGeneration)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PortStatus,
    (std::string, error)
    (int, port)
    (std::string, protocol)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::LoadBalancerIngress,
    (std::string, hostname)
    (std::string, ip)
    (std::vector<k8deployer::k8api::PortStatus>, ports)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::LoadBalancerStatus,
    (std::vector<k8deployer::k8api::LoadBalancerIngress>, ingress)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ServiceStatus,
    (std::vector<k8deployer::k8api::Condition>, conditions)
    (std::optional<k8deployer::k8api::LoadBalancerStatus>, loadBalancer)
)

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Service,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::ServiceSpec, spec)
    (std::optional<k8deployer::k8api::ServiceStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ConfigMap,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (bool, immutable)
    (k8deployer::k8api::key_values_t, data)
    (k8deployer::k8api::key_values_t, binaryData) // base64 encoded
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Secret,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::key_values_t, data)
    (bool, immutable)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::key_values_t, stringData)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::RollingUpdateStatefulSetStrategy,
    (size_t, partition)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::StatefulSetUpdateStrategy,
    (std::optional<k8deployer::k8api::RollingUpdateStatefulSetStrategy>, rollingUpdate)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::TypedLocalObjectReference,
    (std::string, apiGroup)
    (std::string, kind)
    (std::string, name)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeClaimCondition,
    (std::string, lastProbeTime)
    (std::string, lastTransitionTime)
    (std::string, message)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeClaimStatus,
    (k8deployer::k8api::string_list_t, accessModes)
    (k8deployer::k8api::key_values_t, capacity)
    (std::vector<k8deployer::k8api::PersistentVolumeClaimCondition>, conditions)
    (std::string, phase)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeClaimSpec,
    (k8deployer::k8api::string_list_t, accessModes)
    (k8deployer::k8api::TypedLocalObjectReference, dataSource)
    (std::optional<k8deployer::k8api::ResourceRequirements>, resources)
    (k8deployer::k8api::LabelSelector, selector)
    (std::string, storageClassName)
    (std::string, volumeMode)
    (std::string, volumeName)
    (std::optional<k8deployer::k8api::PersistentVolumeClaimStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PersistentVolumeClaim,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::PersistentVolumeClaimSpec, spec)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::StatefulSetSpec,
    (std::string, podManagementPolicy)
    (std::optional<size_t>, replicas)
    (std::optional<size_t>, revisionHistoryLimit)
    (std::optional<k8deployer::k8api::LabelSelector>, selector)
    (std::string, serviceName)
    (std::optional<k8deployer::k8api::PodTemplateSpec>, template_)
    (std::optional<k8deployer::k8api::StatefulSetUpdateStrategy>, updateStrategy)
    (std::vector<k8deployer::k8api::PersistentVolumeClaim>, volumeClaimTemplates)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::StatefulSetCondition,
    (std::string, lastTransitionTime)
    (std::string, gmessage)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::StatefulSetStatus,
    (size_t, collisionCount)
    (std::vector<k8deployer::k8api::StatefulSetCondition>, conditions)
    (size_t, currentReplicas)
    (std::string, currentRevision)
    (size_t, observedGeneration)
    (size_t, readyReplicas)
    (size_t, replicas)
    (std::string, updateRevision)
    (size_t, updatedReplicas)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::StatefulSet,
    (std::string, apiVersion)
    (std::string, kind)
    (std::optional<k8deployer::k8api::ObjectMeta>, metadata)
    (std::optional<k8deployer::k8api::StatefulSetSpec>, spec)
    (std::optional<k8deployer::k8api::StatefulSetStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ServiceBackendPort,
    (std::string, name)
    (std::optional<int>, number)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::IngressServiceBackend,
    (std::string, name)
    (k8deployer::k8api::ServiceBackendPort, port)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::IngressBackend,
    (std::optional<k8deployer::k8api::TypedLocalObjectReference>, resource)
    (std::optional<k8deployer::k8api::IngressServiceBackend>, service)
    (std::string, serviceName)
    (std::string, servicePort)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::HTTPIngressPath,
    (std::optional<k8deployer::k8api::IngressBackend>, backend)
    (std::string, path)
    (std::string, pathType)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::HTTPIngressRuleValue,
    (std::vector<k8deployer::k8api::HTTPIngressPath>, paths)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::IngressRule,
    (std::string, host)
    (std::optional<k8deployer::k8api::HTTPIngressRuleValue>, http)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::IngressTLS,
    (k8deployer::k8api::string_list_t, hosts)
    (std::string, secretName)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::IngressSpec,
    (std::optional<k8deployer::k8api::IngressBackend>, defaultBackend)
    (std::string, ingressClassName)
    (std::vector<k8deployer::k8api::IngressRule>, rules)
    (std::vector<k8deployer::k8api::IngressTLS>, tls)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::IngressStatus,
    (k8deployer::k8api::LoadBalancerStatus, loadBalancer)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Ingress,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (std::optional<k8deployer::k8api::IngressSpec>, spec)
    (std::optional<k8deployer::k8api::IngressStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NamespaceSpec,
    (k8deployer::k8api::string_list_t, finalizers)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NamespaceCondition,
    (std::string, lastTransitionTime)
    (std::string, message)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::NamespaceStatus,
    (std::vector<k8deployer::k8api::NamespaceCondition>, conditions)
    (std::string, phase)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Namespace,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (std::optional<k8deployer::k8api::NamespaceSpec>, spec)
    (std::optional<k8deployer::k8api::NamespaceStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::RollingUpdateDaemonSet,
    (size_t, maxUnavailable)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DaemonSetUpdateStrategy,
    (std::optional<k8deployer::k8api::RollingUpdateDaemonSet>, rollingUpdate)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DaemonSetSpec,
    (size_t, minReadySeconds)
    (size_t, revisionHistoryLimit)
    (k8deployer::k8api::LabelSelector, selector)
    (k8deployer::k8api::PodTemplateSpec, template_)
    (std::optional<k8deployer::k8api::DaemonSetUpdateStrategy>, updateStrategy)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DaemonSetCondition,
    (std::string, lastTransitionTime)
    (std::string, message)
    (std::string, reason)
    (std::string, status)
    (std::string, type)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DaemonSetStatus,
    (size_t, collisionCount)
    (std::vector<k8deployer::k8api::DaemonSetCondition>, conditions)
    (size_t, currentNumberScheduled)
    (size_t, desiredNumberScheduled)
    (size_t, numberAvailable)
    (size_t, numberMisscheduled)
    (size_t, numberReady)
    (size_t, numberUnavailable)
    (size_t, observedGeneration)
    (size_t, updatedNumberScheduled)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DaemonSet,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (std::optional<k8deployer::k8api::DaemonSetSpec>, spec)
    (std::optional<k8deployer::k8api::DaemonSetStatus>, status)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PolicyRule,
    (k8deployer::k8api::string_list_t, apiGroups)
    (k8deployer::k8api::string_list_t, nonResourceURLs)
    (k8deployer::k8api::string_list_t, resourceNames)
    (k8deployer::k8api::string_list_t, resources)
    (k8deployer::k8api::string_list_t, verbs)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Role,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (std::vector<k8deployer::k8api::PolicyRule>, rules)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::RoleRef,
    (std::string, apiGroup)
    (std::string, kind)
    (std::string, name)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Subject,
    (std::string, apiGroup)
    (std::string, kind)
    (std::string, name)
    (std::string, namespace_)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::RoleBinding,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::RoleRef, roleRef)
    (std::vector<k8deployer::k8api::Subject>, subjects)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::AggregationRule,
    (std::vector<k8deployer::k8api::LabelSelector>, clusterRoleSelectors)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ClusterRole,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (std::optional<k8deployer::k8api::AggregationRule>, aggregationRule)
    (std::vector<k8deployer::k8api::PolicyRule>, rules)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ClusterRoleBinding,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::RoleRef, roleRef)
    (std::vector<k8deployer::k8api::Subject>, subjects)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::ServiceAccount,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (std::optional<bool>, automountServiceAccountToken)
    (std::vector<k8deployer::k8api::LocalObjectReference>, imagePullSecrets)
    (std::vector<k8deployer::k8api::ObjectReference>, secrets)
);
