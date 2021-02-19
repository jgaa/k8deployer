
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/NamespaceComponent.h"
#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {


NamespaceComponent::NamespaceComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::NAMESPACE;
    parentRelation_ = ParentRelation::BEFORE;
}


void NamespaceComponent::prepareDeploy()
{
    if (namespace_.metadata.name.empty()) {
        namespace_.metadata.name = name;
    }

    // Apply recursively
    Component::prepareDeploy();
}

bool NamespaceComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::Namespace>(*this, getAccessUrl(),
            [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::Namespace>& /*object*/, K8ObjectState state) {
                if (auto self = wself.lock()) {
                    assert(fn);
                    fn(state);
                }
            }, [](const auto& data) {
                if (data.status) {
                    return data.status->phase == "Active";
                }
                return false;
            }
        );
    }

    return true;
}

void NamespaceComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void NamespaceComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void NamespaceComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    if (auto t = task.lock()) {
        t->startProbeAfterApply = true;
        t->dontFailIfAlreadyExists = true;
    }
    sendApply(namespace_, getCreationUrl(), task);
}

void NamespaceComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string NamespaceComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl() + "/api/v1/namespaces";
    return url;
}

} // ns
