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

        sendProbe<k8api::Job>(*this, url, [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::Job>& /*object*/, K8ObjectState state) {
            if (auto self = wself.lock()) {
                assert(fn);
                fn(state);
            }
        });
    }

    return true;
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
