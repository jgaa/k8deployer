#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class NamespaceComponent : public Component
{
public:
    NamespaceComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

    void prepareDeploy() override;
    bool probe(std::function<void (K8ObjectState)>) override;
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
    std::string getAccessUrl() const override;
};

} // ns

