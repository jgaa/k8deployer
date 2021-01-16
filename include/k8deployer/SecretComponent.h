#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class SecretComponent : public Component
{
public:
    SecretComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

    void prepareDeploy() override;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;

private:
    void doDeploy(std::weak_ptr<Task> task);
    void doRemove(std::weak_ptr<Task> task);

    bool prepared_ = false;
    std::string fromFile_;
};

} // ns

