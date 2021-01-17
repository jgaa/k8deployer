#pragma once

#include "k8deployer/BaseComponent.h"

#include "k8deployer/buildDependencies.h"

namespace k8deployer {

class DeploymentComponent;

template <typename T>
void buildDependenciesT() {}

template <typename T>
void buildDependenciesT<DeploymentComponent>();

class DeploymentComponent : public BaseComponent
{
public:
    DeploymentComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : BaseComponent(parent, cluster, data)
    {
        kind_ = Kind::DEPLOYMENT;
    }

    void prepareDeploy() override;

protected:
    k8api::ObjectMeta *getMetadata() override {
        return &deployment.metadata;
    }

    k8api::PodTemplateSpec *getPodTemplate() override {
        return &deployment.spec.template_;
    }

    k8api::LabelSelector *getLabelSelector() override {
        return &deployment.spec.selector;
    }

    size_t getReplicas() const override {
        return deployment.spec.replicas;
    }

    void buildDependencies() override;
    void doDeploy(std::weak_ptr<Task> task) override;
    void doRemove(std::weak_ptr<Task> task) override;

    friend class k8deployer::buildDependenciesT<DeploymentComponent>();

    // Component interface
public:
    bool probe(std::function<void (K8ObjectState)>) override;
};

} // ns


