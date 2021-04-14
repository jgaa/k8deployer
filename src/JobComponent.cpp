#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/JobComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

void JobComponent::prepareDeploy()
{
    basicPrepareDeploy();

    if (job.spec.template_.spec.restartPolicy.empty()) {
        job.spec.template_.spec.restartPolicy = "Never";
    }

    // Apply recursively
    BaseComponent::prepareDeploy();
}

bool JobComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        const auto url = cluster_->getUrl()
                + "/apis/batch/v1/namespaces/"s
                + job.metadata.namespace_
                + "/jobs/" + name;

        sendProbe<k8api::Job>(*this, url,
            [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::Job>& /*object*/, K8ObjectState state) {
                if (auto self = wself.lock()) {
                    assert(fn);
                    fn(state);
                }
            }, [] (const auto& data) {
                if (Engine::mode() == Engine::Mode::DEPLOY) {
                    for(const auto& cond : data.status->conditions) {
                        if (cond.type == "Complete" && cond.status == "True") {
                            return true;
                        }
                    }
                }
                return false;
            }
        );
    }

    return true;
}

void JobComponent::addRemovementTasks(Component::tasks_t &tasks)
{
    // Remove
    auto removeTask = make_shared<Task>(*this, name + "-delete", [&](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            doRemove(task.weak_from_this());
            task.setState(Task::TaskState::WAITING);
        }

        task.evaluate();
    }, Task::TaskState::PRE, Mode::REMOVE);

    tasks.push_back(removeTask);

    // Redmove old job pods
    // Delete pvc
    auto removePodsTask = make_shared<Task>(*this, name + "-delete-pods", [&, appName=name](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            const auto url = cluster_->getUrl() + "/api/v1/namespaces/" + getNamespace() + "/pods";
            sendDelete(url, task.weak_from_this(), true, {{"labelSelector"s,"app="s + appName},
                                                          {"propagationPolicy", "Orphan"}});
            task.setState(Task::TaskState::WAITING);
        }

        task.evaluate();
    }, Task::TaskState::PRE, Mode::REMOVE);

    tasks.push_back(removePodsTask);
}

void JobComponent::doDeploy(std::weak_ptr<Task> task)
{
    const auto url = cluster_->getUrl()
            + "/apis/batch/v1/namespaces/"s
            + job.metadata.namespace_
            + "/jobs";
    sendApply(job, url, task);
}

void JobComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    const auto url = cluster_->getUrl()
            + "/apis/batch/v1/namespaces/"s
            + job.metadata.namespace_
            + "/jobs/" + name;

    sendDelete(url, task, true);
}

} // ns
