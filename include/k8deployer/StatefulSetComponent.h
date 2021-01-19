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
    void addRemovementTasks(tasks_t &tasks) override;

    k8api::ObjectMeta *getMetadata() override {
        if (statefulSet.metadata) {
            return &statefulSet.metadata.value();
        }
        return {};
    }

    k8api::PodTemplateSpec *getPodTemplate() override {
        if (statefulSet.spec && statefulSet.spec->template_) {
            return &statefulSet.spec->template_.value();
        }
        return {};
    }

    k8api::LabelSelector *getLabelSelector() override {
        if (statefulSet.spec && statefulSet.spec->selector) {
            return &statefulSet.spec->selector.value();
        }
        return {};
    }

    size_t getReplicas() const override {
        if (statefulSet.spec && statefulSet.spec->replicas) {
            return *statefulSet.spec->replicas;
        }
        return 1;
    }

    k8api::StatefulSetSpec *getSpec(bool create = true) {
        if (!statefulSet.spec) {
            if (!create) {
                return {};
            }
            statefulSet.spec.emplace();
        }
        return &statefulSet.spec.value();
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




