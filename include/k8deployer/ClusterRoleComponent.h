#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class ClusterRoleComponent : public Component
{
public:
    ClusterRoleComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

    void prepareDeploy() override;
    std::string getNamespace() const override {
        return {};
    }

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;

private:
    void doDeploy(std::weak_ptr<Task> task);
    void doRemove(std::weak_ptr<Task> task);
    std::string getCreationUrl() const override;
};

} // ns

