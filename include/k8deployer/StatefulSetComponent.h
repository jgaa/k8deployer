#pragma once

#include "k8deployer/DeploymentComponent.h"

namespace k8deployer {

class StatefulSetComponent : public DeploymentComponent
{
public:
    StatefulSetComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : DeploymentComponent(parent, cluster, data)
    {
        kind_ = Kind::STATEFULSET;
    }

    void prepareDeploy() override;

protected:
    k8api::ObjectMeta *getMetadata() override {
        return &statefulSet.metadata;
    }

    k8api::PodTemplateSpec *getPodTemplate() override {
        return &statefulSet.spec.template_;
    }

    k8api::LabelSelector *getLabelSelector() override {
        return &statefulSet.spec.selector;
    }

    size_t getReplicas() const override {
        return statefulSet.spec.replicas;
    }

    void buildDependencies() override;
    void doDeploy(std::weak_ptr<Task> task) override;
    void doRemove(std::weak_ptr<Task> task) override;
    std::string getCreationUrl() const override;

    // Component interface
public:
    bool probe(std::function<void (K8ObjectState)>) override;
};

} // ns




