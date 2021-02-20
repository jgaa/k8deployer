
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/RoleBindingComponent.h"
#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {


RoleBindingComponent::RoleBindingComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::ROLEBINDING;
    parentRelation_ = ParentRelation::BEFORE;
}

void RoleBindingComponent::prepareDeploy()
{
    if (rolebinding.metadata.name.empty()) {
        rolebinding.metadata.name = name;
    }

    // Apply recursively
    Component::prepareDeploy();
}

void RoleBindingComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void RoleBindingComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void RoleBindingComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    sendApply(rolebinding, getCreationUrl(), task);
}

void RoleBindingComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string RoleBindingComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl() + "/apis/" + rolebinding.apiVersion + "/namespaces/" + getNamespace()
            + "/rolebindings";
    return url;
}



} // ns
