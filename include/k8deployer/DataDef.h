#pragma once

#include <deque>
#include "k8deployer/k8/k8api.h"

namespace k8deployer {

using conf_t = std::map<std::string, std::string>;
using labels_t = std::map<std::string, std::string>;
using variables_t = std::map<std::string, std::string>;

struct StorageDef {
    k8api::VolumeMount volume;
    std::string capacity;
    bool createVolume = false;

    // Use init-container to set permissions on the volume
    // Tis is a work-around for k8s' hopelessly broken hostPath
    std::string chownUser;
    std::string chownGroup;
    std::string chmodMode;
};

struct ComponentData {
    virtual ~ComponentData() = default;

    std::string name;
    bool enabled = true;
    labels_t labels;
    conf_t defaultArgs; // Added to args and childrens args, unless overridden
    conf_t args;
    k8api::string_list_t depends;

    // Can be populated by configuration, but normally we will do it
    k8api::Job job;
    k8api::Deployment deployment;
    k8api::StatefulSet statefulSet;
    k8api::DaemonSet daemonset;
    k8api::Service service;
    k8api::ConfigMap configmap;
    std::optional<k8api::Secret> secret;
    k8api::PersistentVolume persistentVolume;
    k8api::Ingress ingress;
    k8api::Namespace namespace_;
    k8api::Role role;
    k8api::ClusterRole clusterrole;
    k8api::RoleBinding rolebinding;
    k8api::ClusterRoleBinding clusterrolebinding;
    k8api::ServiceAccount serviceaccount;

    // Set on pods if defined on components using podspecs
    std::optional<k8api::SecurityContext> podSecurityContext;

    // Set on podspec if defined on components using podspecs
    std::optional<k8api::PodSecurityContext> podSpecSecurityContext;

    // Applied to the container if it's indirectly created by k8deployer
    std::optional<k8api::Probe> startupProbe;
    std::optional<k8api::Probe> livenessProbe;
    std::optional<k8api::Probe> readinessProbe;

    std::vector<StorageDef> storage;
};

struct ComponentDataDef : public ComponentData {
    // Related components owned by this; like volumes or configmaps or service.

    std::string kind;
    std::string parentRelation;

    using childrens_t = std::deque<ComponentDataDef>;
    childrens_t children;
};

} // ns
