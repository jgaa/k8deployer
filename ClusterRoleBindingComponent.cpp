
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/ClusterRoleBindingComponent.h"
#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {


ClusterRoleBindingComponent::ClusterRoleBindingComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::CLUSTERROLEBINDING;
    parentRelation_ = ParentRelation::BEFORE;
}

void ClusterRoleBindingComponent::prepareDeploy()
{
    if (clusterrolebinding.metadata.name.empty()) {
        clusterrolebinding.metadata.name = name;
    }

    for(auto& s: clusterrolebinding.subjects) {
        if (s.namespace_.empty()) {
            s.namespace_ = getNamespace();
        }
    }

    // Apply recursively
    Component::prepareDeploy();
}

void ClusterRoleBindingComponent::addDeploymentTasks(Component::tasks_t &tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event */*event*/) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            doDeploy(task.weak_from_this());
        }

        task.evaluate();
    });

    tasks.push_back(task);
    Component::addDeploymentTasks(tasks);
}

void ClusterRoleBindingComponent::addRemovementTasks(Component::tasks_t &tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event */*event*/) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            doRemove(task.weak_from_this());
        }

        task.evaluate();
    }, Task::TaskState::READY);

    tasks.push_back(task);
    Component::addRemovementTasks(tasks);
}

void ClusterRoleBindingComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    sendApply(clusterrolebinding, getCreationUrl(), task);
}

void ClusterRoleBindingComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string ClusterRoleBindingComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl() + "/apis/" + clusterrolebinding.apiVersion
            + "/clusterrolebindings";
    return url;
}



} // ns
