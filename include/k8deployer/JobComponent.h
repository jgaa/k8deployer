#pragma once

#include "k8deployer/BaseComponent.h"

namespace k8deployer {

class JobComponent : public BaseComponent
{
public:
    JobComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : BaseComponent(parent, cluster, data)
    {
        kind_ = Kind::JOB;
    }

    std::future<void> prepareDeploy() override;

protected:
    k8api::ObjectMeta *getMetadata() override {
        return &job.metadata;
    }

    k8api::PodTemplateSpec *getPodTemplate() override {
        return &job.spec.template_;
    }

    k8api::LabelSelector *getLabelSelector() override {
        return {}; //&job.spec.selector;
    }

    void buildDependencies();
    void doDeploy(std::weak_ptr<Task> task) override;
    void doRemove(std::weak_ptr<Task> task) override;
};

} // ns


