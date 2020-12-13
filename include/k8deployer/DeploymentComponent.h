#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class DeploymentComponent : public Component
{
public:
    DeploymentComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {}

    std::future<void> prepareDeploy() override;

protected:
    void addTasks(tasks_t& tasks) override;

private:
    void buildDependencies();
    void doDeploy();

    size_t podsStarted_ = 0;
};

} // ns

