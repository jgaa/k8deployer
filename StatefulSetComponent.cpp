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
