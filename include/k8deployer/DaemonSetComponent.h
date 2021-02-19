#pragma once

#include "k8deployer/DeploymentComponent.h"

namespace k8deployer {

class DaemonSetComponent : public DeploymentComponent
{
public:
    DaemonSetComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : DeploymentComponent(parent, cluster, data)
    {
        kind_ = Kind::DAEMONSET;
    }

    void prepareDeploy() override;

protected:
    void addRemovementTasks(tasks_t &tasks) override;

    k8api::ObjectMeta *getMetadata() override {
        return &daemonset.metadata;
    }

    k8api::PodTemplateSpec *getPodTemplate() override {
        return &getSpec(true)->template_;
    }

    k8api::LabelSelector *getLabelSelector() override {
        return &getSpec(true)->selector;
    }

    size_t getReplicas() const override {
        return 1;
    }

    k8api::DaemonSetSpec *getSpec(bool create = true) {
        if (!daemonset.spec) {
            if (!create) {
                return {};
            }
            daemonset.spec.emplace();
        }

        return &daemonset.spec.value();
    }

    //void buildDependencies() override;
    void doDeploy(std::weak_ptr<Task> task) override;
    void doRemove(std::weak_ptr<Task> task) override;
    std::string getCreationUrl() const override;

    // Component interface
public:
    bool probe(std::function<void (K8ObjectState)>) override;
};

} // ns




