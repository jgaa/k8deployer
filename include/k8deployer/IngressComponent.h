#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class IngressComponent : public Component
{
public:
    IngressComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {
        kind_ = Kind::INGRESS;
        parentRelation_ = ParentRelation::AFTER;
    }

    void prepareDeploy() override;

    bool probe(std::function<void(K8ObjectState state)>) override;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;

private:
    void doDeploy(std::weak_ptr<Task> task);
    void doRemove(std::weak_ptr<Task> task);
    std::string getCreationUrl() const override;
    std::string loadBalancerIp_;
};

} // ns



class IngressComponent
{
public:
    IngressComponent();
};

