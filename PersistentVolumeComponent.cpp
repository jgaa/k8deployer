#include "include/k8deployer/PersistentVolumeComponent.h"

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/PersistentVolumeComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Component.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

PersistentVolumeComponent::PersistentVolumeComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::PERSISTENTVOLUME;
    parentRelation_ = ParentRelation::BEFORE;
}

void PersistentVolumeComponent::prepareDeploy()
{
    if (auto st = cluster_->getStorage()) {
        persistentVolume = st->createNewVolume(getArg("pv.capacity", "1Gi"), *this);
    }

    if (configmap.metadata.name.empty()) {
        persistentVolume.metadata.name = name;
    }

    if (configmap.metadata.namespace_.empty()) {
        persistentVolume.metadata.namespace_ = getNamespace();
    }

    persistentVolume.spec.claimRef["namespace"] = getNamespace();
    persistentVolume.spec.claimRef["name"] = name;

    // Apply recursively
    Component::prepareDeploy();
}

bool PersistentVolumeComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::PersistentVolume>(*this, getAccessUrl(),
            [wself=weak_from_this(), fn=move(fn)]
            (const std::optional<k8api::PersistentVolume>& /*object*/, K8ObjectState state) {
                if (auto self = wself.lock()) {
                    assert(fn);
                    fn(state);
                }
            }, [] (const auto& data) {
                if (Engine::mode() == Engine::Mode::DEPLOY) {
                    return data.status->phase == "Available";
                }
                return false;
            }
        );
    }

    return true;

}

void PersistentVolumeComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void PersistentVolumeComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void PersistentVolumeComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    if (auto t = task.lock()) {
        t->startProbeAfterApply = true;
    }
    sendApply(persistentVolume, getCreationUrl(), task);
}

void PersistentVolumeComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string PersistentVolumeComponent::getCreationUrl() const
{
    static const auto url = cluster_->getUrl() + "/api/v1/persistentvolumes";
    return url;
}



} // ns
