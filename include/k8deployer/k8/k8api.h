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
    std::optional<Time> timeAdded;
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
    HostPathVolumeSource hostPath;
    LocalVolumeSource local;
    NFSVolumeSource nfs;
    string_list_t mountOptions;
    VolumeNodeAffinity nodeAffinity;
    std::string persistentVolumeReclaimPolicy;
    std::string storageClassName;
    std::string volumeMode;
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
    int mode = 0644;
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

struct Volume {
    std::string name;
    ConfigMapVolumeSource configMap;
    // TODO: Make optional
    //EmptyDirVolumeSource emptyDir;
    //HostPathVolumeSource hostPath;
    //NFSVolumeSource nfs;
    //PersistentVolumeClaimVolumeSource persistentVolumeClaim;
};

using volumes_t = std::vector<Volume>;

struct HostAlias {
    string_list_t hostnames;
    std::string ip;
};

using host_aliases_t = std::vector<HostAlias>;

struct PodSpec {
    std::optional<Affinity> affinity;
    containers_t containers;
    bool enableServiceLinks = true;
    std::string hostname;
    local_object_references_t imagePullSecrets;
    containers_t initContainers;
    std::string nodeName;
    std::string restartPolicy;
    tolology_spread_constraints_t topologySpreadConstraints;
    bool hostPID = false;
    volumes_t volumes;
    host_aliases_t hostAliases;
};

struct PodTemplateSpec {
    ObjectMeta metadata;
    PodSpec spec;
};

struct DeploymentSpec {
    size_t replicas = 1;
    LabelSelector selector;
    DeploymentStrategy strategy;
    PodTemplateSpec template_; // NB:
};


struct Deployment {
    std::string apiVersion = "apps/v1";
    std::string kind = "Deployment";
    ObjectMeta metadata;
    DeploymentSpec spec;
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

struct Service {
    std::string apiVersion = "v1";
    std::string kind = "Service";
    ObjectMeta metadata;
    ServiceSpec spec;
};

struct ConfigMap {
    std::string apiVersion = "v1";
    std::string kind = "ConfigMap";
    ObjectMeta metadata;
    bool immutable = false;
    key_values_t data;
    key_values_t binaryData; // base64 encoded
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
    (std::optional<k8deployer::k8api::Time>, timeAdded)
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
    (k8deployer::k8api::HostPathVolumeSource, hostPath)
    (k8deployer::k8api::LocalVolumeSource, local)
    (k8deployer::k8api::NFSVolumeSource, nfs)
    (k8deployer::k8api::string_list_t, mountOptions)
    (k8deployer::k8api::VolumeNodeAffinity, nodeAffinity)
    (std::string, persistentVolumeReclaimPolicy)
    (std::string, storageClassName)
    (std::string, volumeMode)
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
    (int, mode)
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

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Volume,
    (std::string, name)
    (k8deployer::k8api::ConfigMapVolumeSource, configMap)
    //(k8deployer::k8api::EmptyDirVolumeSource, emptyDir)
    //(k8deployer::k8api::HostPathVolumeSource, hostPath)
    //(k8deployer::k8api::NFSVolumeSource, nfs)
    //(k8deployer::k8api::PersistentVolumeClaimVolumeSource, persistentVolumeClaim)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::HostAlias,
    (k8deployer::k8api::string_list_t, hostnames)
    (std::string, ip)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodSpec,
    (std::optional<k8deployer::k8api::Affinity>, affinity)
    (k8deployer::k8api::containers_t, containers)
    (bool, enableServiceLinks)
    (std::string, hostname)
    (k8deployer::k8api::local_object_references_t, imagePullSecrets)
    (k8deployer::k8api::containers_t, initContainers)
    (std::string, nodeName)
    (std::string, restartPolicy)
    (k8deployer::k8api::tolology_spread_constraints_t, topologySpreadConstraints)
    (bool, hostPID)
    (k8deployer::k8api::volumes_t, volumes)
    (k8deployer::k8api::host_aliases_t, hostAliases)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::PodTemplateSpec,
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::PodSpec, spec)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::DeploymentSpec,
    (size_t, replicas)
    (k8deployer::k8api::LabelSelector, selector)
    (k8deployer::k8api::DeploymentStrategy, strategy)
    (k8deployer::k8api::PodTemplateSpec, template_), //, NB:
);


BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Deployment,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::DeploymentSpec, spec)
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

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Service,
    (std::string, apiVersion)
    (std::string, kind)
    (k8deployer::k8api::ObjectMeta, metadata)
    (k8deployer::k8api::ServiceSpec, spec)
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
