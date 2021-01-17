#include "include/k8deployer/StatefulSetComponent.h"

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
    basicPrepareDeploy();
    buildDependencies();

    // Apply recursively
    return BaseComponent::prepareDeploy();
}

void StatefulSetComponent::buildDependencies()
{
    DeploymentComponent::buildDependencies();

    if (auto replicas = getArg("replicas")) {
        statefulSet.spec.replicas = stoull(*replicas);
    }

    if (const auto svc = getFirstKindAmongChildren(Kind::SERVICE)) {
        statefulSet.spec.serviceName = svc->name;
    }

    if (auto policy = getArg("podManagementPolicy")) {
        statefulSet.spec.podManagementPolicy = *policy;
    }

    for (auto storageDef : storage) {

        if (storageDef.volume.name.empty()) {
            storageDef.volume.name = "storage";
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
            // Since we create the service, give it a copy of relevant arguments..
            for(const auto& [k, v] : args) {
                static const array<string, 2> relevant = {"service.nodePort", "service.type"};
                if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                    svcargs[k] = v;
                }
            }

            for(size_t i = 0; i < statefulSet.spec.replicas; ++i) {
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


        statefulSet.spec.volumeClaimTemplates.push_back(pv);

        if (statefulSet.spec.template_.spec.containers.empty()) {
            throw runtime_error("No containers in StatefulSet when adding storage");
        }
        statefulSet.spec.template_.spec.containers.front().volumeMounts.push_back(
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
    static const auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + statefulSet.metadata.namespace_
            + "/statefulsets";

    return url;
}

bool StatefulSetComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::StatefulSet>(*this, getAccessUrl(), [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::StatefulSet>& /*object*/, K8ObjectState state) {
            if (auto self = wself.lock()) {
                assert(fn);
                fn(state);
            }
        });
    }

    return true;
}


} // ns
