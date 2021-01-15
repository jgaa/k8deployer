#pragma once

#include "k8deployer/BaseComponent.h"

namespace k8deployer {

class DeploymentComponent : public BaseComponent
{
public:
    DeploymentComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : BaseComponent(parent, cluster, data)
    {
        kind_ = Kind::DEPLOYMENT;
    }

    std::future<void> prepareDeploy() override;

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

    void buildDependencies();
    void doDeploy(std::weak_ptr<Task> task) override;
    void doRemove(std::weak_ptr<Task> task) override;
};

} // ns


