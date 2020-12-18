#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class ConfigMapComponent : public Component
{
public:
    ConfigMapComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

    std::future<void> prepareDeploy() override;

protected:
    void addTasks(tasks_t& tasks) override;

private:
    void doDeploy(std::weak_ptr<Task> task);

    size_t podsStarted_ = 0;
    bool prepared_ = false;
};

} // ns

