
#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/DeploymentComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

void DeploymentComponent::prepareDeploy()
{
    if (auto replicas = getArg("replicas")) {
        deployment.spec.replicas = stoull(*replicas);
    }

    basicPrepareDeploy();
    buildDependencies();

    // Apply recursively
    return BaseComponent::prepareDeploy();
}

void DeploymentComponent::buildDependencies()
{
    if (labels.empty()) {
        labels["app"] = name; // Use the name as selector
    }

    // A deployment normally needs a service
    const auto serviceEnabled = getBoolArg("service.enabled");
    if (!hasKindAsChild(Kind::SERVICE) && ((serviceEnabled && *serviceEnabled) || !serviceEnabled)) {
        LOG_DEBUG << logName() << "Adding Service.";

        conf_t svcargs;
        // Since we create the service, give it a copy of relevant arguments..
        for(const auto& [k, v] : args) {
            static const array<string, 2> relevant = {"service.nodePort", "service.type"};
            if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                svcargs[k] = v;
            }
        }

        addChild(name + "-svc", Kind::SERVICE, labels, svcargs);
    }

    // Check for config-files --> ConfigMap
    if (auto fileNames = getArg("config.fromFile")) {
        LOG_DEBUG << logName() << "Adding ConfigMap.";

        conf_t svcargs;
        for(const auto& [k, v] : args) {
            static const array<string, 1> relevant = {"config.fromFile"};
            if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                svcargs[k] = v;
            }
        }

        auto cf = addChild(name + "-conf", Kind::CONFIGMAP, {}, svcargs);
        cf->prepareDeploy(); // We need the fully initialized ConfigMap in order to map the volume

        // Add the configmap as a volume to the first pod
        k8api::PodSpec *podspec = {};
        podspec = &deployment.spec.template_.spec;

        k8api::Volume volume;
        volume.name = cf->configmap.metadata.name;
        volume.configMap.name = cf->configmap.metadata.name;

        for(auto& [k, _] : cf->configmap.binaryData) {
            k8api::KeyToPath ktp;
            ktp.key = k;
            ktp.path = k;
            ktp.mode = 0440;
            volume.configMap.items.push_back(ktp);
        }

        podspec->volumes.push_back(volume);

        // Now, add the mount point to the containers so they can use it
        // Assume `/config` for auto-added volumes from "config.fromFile"
        k8api::VolumeMount vm;
        vm.name = volume.name;
        vm.mountPath = "/config";
        vm.readOnly = true;

        for(auto& container: podspec->containers) {
            container.volumeMounts.push_back(vm);
        }
    }

    // Check for docker hub secrets
    if (auto dh = getArg("imagePullSecrets.fromDockerLogin")) {
        LOG_DEBUG << logName() << "Adding Docker credentials.";

        conf_t svcargs;
        for(const auto& [k, v] : args) {
            static const array<string, 1> relevant = {"magePullSecrets.fromDockerLogin"};
            if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                svcargs[k] = v;
            }
        }

        auto child = addChild(name + "-dhcreds", Kind::SECRET, {}, svcargs);
        k8api::PodSpec *podspec = {};
        podspec = &deployment.spec.template_.spec;
        k8api::LocalObjectReference lor = {child->name};
        podspec->imagePullSecrets.push_back(lor);
    }
}

void DeploymentComponent::doDeploy(std::weak_ptr<Task> task)
{
    const auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + deployment.metadata.namespace_
            + "/deployments";
    sendApply(deployment, url, task);
}

void DeploymentComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    const auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + deployment.metadata.namespace_
            + "/deployments/" + name;

    sendDelete(url, task);
}

bool DeploymentComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        const auto url = cluster_->getUrl()
                + "/apis/apps/v1/namespaces/"
                + deployment.metadata.namespace_
                + "/deployments/" + name;

        sendProbe<k8api::Deployment>(*this, url, [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::Deployment>& /*object*/, K8ObjectState state) {
            if (auto self = wself.lock()) {
                assert(fn);
                fn(state);
            }
        });
    }

    return true;
}


} // ns
