
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/RoleComponent.h"
#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {


RoleComponent::RoleComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::ROLE;
    parentRelation_ = ParentRelation::BEFORE;
}

void RoleComponent::prepareDeploy()
{
    if (role.metadata.name.empty()) {
        role.metadata.name = name;
    }

    // Apply recursively
    Component::prepareDeploy();
}

void RoleComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void RoleComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void RoleComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    sendApply(role, getCreationUrl(), task);
}

void RoleComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string RoleComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl() + "/apis/" + role.apiVersion + "/namespaces/" + getNamespace()
            + "/roles";
    return url;
}



} // ns
