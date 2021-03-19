#pragma once

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/Engine.h"
#include "k8deployer/Component.h"
#include "k8deployer/logging.h"

namespace k8deployer {

class BaseComponent : public Component
{
public:
    BaseComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {}

    virtual k8api::ObjectMeta *getMetadata() = 0;
    virtual k8api::PodTemplateSpec *getPodTemplate() = 0;
    virtual k8api::LabelSelector *getLabelSelector() = 0;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;
    void filterEnvVars(k8api::env_vars_t& vars);

    virtual size_t getReplicas() const {
        return 1;
    }

    void basicPrepareDeploy();
    virtual void buildInitContainers();
    virtual void doDeploy(std::weak_ptr<Task> task) = 0;
    virtual void doRemove(std::weak_ptr<Task> task) = 0;

    size_t podsStarted_ = 0;
};

} // ns

class BaseComponent
{
public:
    BaseComponent();
};

