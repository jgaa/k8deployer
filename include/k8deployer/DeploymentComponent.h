#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class DeploymentComponent : public Component
{
public:
    DeploymentComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {
        kind_ = Kind::DEPLOYMENT;
    }

    std::future<void> prepareDeploy() override;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;

private:
    void buildDependencies();
    void doDeploy(std::weak_ptr<Task> task);
    void doRemove(std::weak_ptr<Task> task);

    size_t podsStarted_ = 0;    
};

} // ns


