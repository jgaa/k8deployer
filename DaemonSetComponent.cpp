#include "include/k8deployer/DaemonSetComponent.h"

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/DaemonSetComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

void DaemonSetComponent::prepareDeploy()
{
    if (!daemonset.spec) {
        statefulSet.spec.emplace();
    }

    basicPrepareDeploy();
    buildDependencies();

    // Apply recursively
    return BaseComponent::prepareDeploy();
}

void DaemonSetComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void DaemonSetComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    sendApply(daemonset, getCreationUrl(), task);
}

void DaemonSetComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string DaemonSetComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + getNamespace()
            + "/daemonsets";

    return url;
}

bool DaemonSetComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::DaemonSet>(*this, getAccessUrl(),
            [wself=weak_from_this(), fn=move(fn)]
            (const std::optional<k8api::DaemonSet>& /*object*/, K8ObjectState state) {
            if (auto self = wself.lock()) {
                assert(fn);
                fn(state);
            }
            }, [](const auto& data) {
                if (Engine::mode() == Engine::Mode::DELETE) {
                    return false; // Done when deleted
                }
                return data.status->numberReady > 0;
            }
        );
    }

    return true;
}


} // ns
