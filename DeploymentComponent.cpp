
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

string DeploymentComponent::getCreationUrl() const
{
    static const auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + deployment.metadata.namespace_
            + "/deployments";

    return url;
}

void DeploymentComponent::buildDependencies()
{
    assert(getPodTemplate());
    k8api::PodSpec *podspec = &getPodTemplate()->spec;

    if (labels.empty()) {
        labels["app"] = name; // Use the name as selector
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
        auto& podspec = this->getPodTemplate()->spec;
        k8api::LocalObjectReference lor = {child->name};
        podspec.imagePullSecrets.push_back(lor);
    }

    std::string tlsSecretName;
    if (auto tls = getArg("tlsSecret")) {
        LOG_DEBUG << logName() << "Adding tls secret.";

        conf_t tlsargs;
        for(const auto& [k, v] : args) {
            static const array<string, 1> relevant = {"tlsSecret"};
            if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                tlsargs[k] = v;
            }
        }
        auto child = addChild(name + "-tls", Kind::SECRET, {}, tlsargs);
        tlsSecretName = child->name;
        auto& podspec = this->getPodTemplate()->spec;
        k8api::LocalObjectReference lor = {child->name};
        podspec.imagePullSecrets.push_back(lor);
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

        auto service = addChild(name + "-svc", Kind::SERVICE, labels, svcargs, "before");

        if (auto ip = getArg("ingress.paths"); ip && !ip->empty()) {
            // Add ingress
            conf_t iargs;
            if (!tlsSecretName.empty()) {
                // Explicit set "ingress.secret" will override below
                iargs["ingress.secret"] = tlsSecretName;
            }
            for(const auto& [k, v] : args) {
                static const array<string, 2> relevant = {"ingress.secret", "ingress.paths"};
                if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                    iargs[k] = v;
                }
            }

            service->addChild(name + "-ingr", Kind::INGRESS, labels, iargs);
        }
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
        k8api::Volume volume;
        volume.name = cf->configmap.metadata.name;
        volume.configMap = {};
        volume.configMap->name = cf->configmap.metadata.name;

        for(auto& [k, _] : cf->configmap.binaryData) {
            k8api::KeyToPath ktp;
            ktp.key = k;
            ktp.path = k;
            ktp.mode = 0440;
            volume.configMap->items.push_back(ktp);
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
}

void DeploymentComponent::doDeploy(std::weak_ptr<Task> task)
{
    sendApply(deployment, getCreationUrl(), task);
}

void DeploymentComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task);
}

bool DeploymentComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::Deployment>(*this, getAccessUrl(),
            [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::Deployment>& /*object*/, K8ObjectState state) {
                if (auto self = wself.lock()) {
                    assert(fn);
                    fn(state);
                }
            }, [](const auto& data) {
                for(const auto& cond : data.status->conditions) {
                    if (cond.type == "Available" && cond.status == "True") {
                        return true;
                    }
                 }

                return false;
            }
        );
    }

    return true;
}


} // ns
