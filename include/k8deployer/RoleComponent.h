#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class RoleComponent : public Component
{
public:
    RoleComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

    void prepareDeploy() override;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;

private:
    void doDeploy(std::weak_ptr<Task> task);
    void doRemove(std::weak_ptr<Task> task);
    std::string getCreationUrl() const override;
};

} // ns

