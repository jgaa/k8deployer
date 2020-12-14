#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class ServiceComponent : public Component
{
public:
    ServiceComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {
        kind_ = Kind::SERVICE;
    }

    std::future<void> prepareDeploy() override;

protected:
    void addTasks(tasks_t& tasks) override;

private:
    void doDeploy(std::weak_ptr<Task> task);
};

} // ns



