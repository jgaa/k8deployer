#include "k8deployer/StatefulSetComponent.h"
#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/logging.h"
#include "k8deployer/StatefulSetComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

void StatefulSetComponent::prepareDeploy()
{
    if (!getSpec(true)->template_) {
        getSpec(true)->template_.emplace();
    }
    if (!statefulSet.metadata) {
        statefulSet.metadata.emplace();
    }

    if (!statefulSet.spec->selector) {
        statefulSet.spec->selector.emplace();
    }

    basicPrepareDeploy();
    buildDependencies();

    // Apply recursively
    return BaseComponent::prepareDeploy();
}

void StatefulSetComponent::addRemovementTasks(Component::tasks_t &tasks)
{
    // Scale down to zero to dispose storage
    auto scaleDownTask = make_shared<Task>(*this, name + "-scale-down", [&](Task& task,
                                           const k8api::Event *event) {
        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            k8api::StatefulSet ss;
            ss.spec.emplace();
            ss.apiVersion.clear();
            ss.kind.clear();
            ss.spec->replicas = 0;
            task.startProbeAfterApply = true;
            sendApply(move(ss), getAccessUrl() ,task.weak_from_this(),
                      restc_cpp::Request::Type::PATCH);
            task.setState(Task::TaskState::WAITING);
        }

        task.evaluate();
    }, Task::TaskState::READY, Mode::REMOVE);
    tasks.push_back(scaleDownTask);

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

    removeTask->addDependency(scaleDownTask);
    tasks.push_back(removeTask);


    // Delete pvc
    auto removePvcTask = make_shared<Task>(*this, name + "-delete-pvc", [&, appName=name](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            const auto url = cluster_->getUrl() + "/api/v1/namespaces/" + getNamespace() + "/persistentvolumeclaims";
            sendDelete(url, task.weak_from_this(), true, {{"labelSelector"s,"app="s + appName},
                                                          {"propagationPolicy", "Orphan"}});
            task.setState(Task::TaskState::WAITING);
        }

        task.evaluate();
    }, Task::TaskState::PRE, Mode::REMOVE);

    removeTask->addDependency(removePvcTask);
    tasks.push_back(removePvcTask);

    Component::addRemovementTasks(tasks);
}

void StatefulSetComponent::buildDependencies()
{
    DeploymentComponent::buildDependencies();

    if (auto replicas = getArg("replicas")) {
        getSpec()->replicas = stoull(*replicas);
    }

    if (const auto svc = getFirstKindAmongChildren(Kind::SERVICE)) {
        getSpec()->serviceName = svc->name;
    }

    if (auto policy = getArg("podManagementPolicy")) {
        getSpec()->podManagementPolicy = *policy;
    }

    for (auto& storageDef : storage) {

        if (storageDef.volume.name.empty()) {
            // Provisioned volumes are not assigned to a namespace, so in order
            // to get their names unique when deploing in parallell to multiple clusters
            // we have to add the namespace.
            storageDef.volume.name = getNamespace() + "-storage";
        }

        if (storageDef.capacity.empty()) {
            storageDef.capacity = "100Mi";
            LOG_WARN << "In storage \"" << storageDef.volume.name
                     << "\" there is no capacity defined. Using "
                     << storageDef.capacity;
        }

        if (storageDef.createVolume) {
            auto storage = cluster_->getStorage();
            if (!storage) {
                LOG_ERROR << logName() << "createVolume is true, but no storage engine is active.";
                throw runtime_error("Missing storageEngine");
            }

            conf_t svcargs;
            // Special arg to propagate the capacity to the pv
            svcargs["pv.capacity"] = storageDef.capacity;

            for(size_t i = 0; i < getReplicas(); ++i) {
                addChild(storageDef.volume.name + "-" + name + "-" + to_string(i),
                         Kind::PERSISTENTVOLUME, {}, svcargs);
            }
        }

        k8api::PersistentVolumeClaim pv;
        pv.metadata.namespace_ = getNamespace();
        pv.metadata.name = storageDef.volume.name;
        pv.spec.accessModes = {"ReadWriteOnce"};
        pv.spec.resources = k8api::ResourceRequirements{};
        pv.spec.resources->requests["storage"] = storageDef.capacity;


        statefulSet.spec->volumeClaimTemplates.push_back(pv);

        if (statefulSet.spec->template_->spec.containers.empty()) {
            throw runtime_error("No containers in StatefulSet when adding storage");
        }
        statefulSet.spec->template_->spec.containers.front().volumeMounts.push_back(
                    storageDef.volume);
    }
}

void StatefulSetComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    sendApply(statefulSet, getCreationUrl(), task);
}

void StatefulSetComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string StatefulSetComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + getNamespace()
            + "/statefulsets";

    return url;
}

bool StatefulSetComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::StatefulSet>(*this, getAccessUrl(),
            [wself=weak_from_this(), fn=move(fn)]
            (const std::optional<k8api::StatefulSet>& /*object*/, K8ObjectState state) {
            if (auto self = wself.lock()) {
                assert(fn);
                fn(state);
            }
            }, [](const auto& data) {

                // TODO: We may have to allow a lower number of replicas to signal ready...
                const size_t replicas = (data.spec->replicas ? *data.spec->replicas : 1);

                LOG_TRACE << "Probe verify: replicas = " << replicas
                          << ", readyReplicas = " << (data.status ? to_string(data.status->readyReplicas) : "[null]"s);

                if (Engine::mode() == Engine::Mode::DELETE) {
                    return data.status->readyReplicas == 0;
                }

                return data.status->readyReplicas == replicas;
            }
        );
    }

    return true;
}


} // ns
