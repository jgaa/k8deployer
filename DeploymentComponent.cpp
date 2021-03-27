
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
    const auto url = cluster_->getUrl()
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

        auto ports = parsePorts(getArg("port", ""));

        // Split the ports into the potential different service types we need.
        std::multimap<std::string, decltype(ports)> service_ports;
        std::string override = getArg("service.type", "");
        for(const auto& p : ports) {
            if (!p.serviceType.empty()) {
                override = p.serviceType;
            }
            string key = "ClusterIP";
            if (override.empty()) {
                if (p.nodePort) {
                    key = "NodePort";
                }
            } else {
                key = override;
            }

            // Create unique services for type and specified service name
            auto svc_name = p.getServiceName(name);

            // If we already have the same key and the same serviceName,
            // just add the port.
            bool added = false;
            auto range = service_ports.equal_range(key);
            for(auto it = range.first; it != range.second; ++it) {
                if (it->second.front().getServiceName(name) == svc_name) {
                    it->second.emplace_back(p);
                    added = true;
                    break;
                }
            }

            if (!added) {
                decltype (ports) pp;
                pp.push_back(p);
                service_ports.insert({key, pp});
            }
        }

        // Now, create the service(s) we need
        int count = -1;
        bool did_ingress = false;
        bool do_ingress = service_ports.size() == 1;
        for(const auto& [type, port] : service_ports) {
            ++count; // First is 0
            conf_t svcargs;
            // Since we create the service, give it a copy of relevant arguments..
            for(const auto& [k, v] : args) {
                static const array<string, 3> relevant = {"service.nodePort", "service.type", "ingress.paths"};
                if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                    svcargs[k] = v;
                }
            }

            // Create args for the ports for this service.
            string pargs;
            for(const auto& pa : port) {
                string c = to_string(pa.port);
                if (!pa.name.empty()) {
                    c += ":name="s + pa.name;
                }
                if (pa.protocol != "TCP") {
                    c += ":protocol="s + pa.protocol;
                }
                if (pa.nodePort) {
                    c += ":nodePort="s + to_string(*pa.nodePort);
                }
                if (!pa.serviceName.empty()) {
                    c += ":serviceName=" + pa.serviceName;
                }
                if (pa.ingress) {
                    c += ":ingress";
                    do_ingress = true;
                    svcargs["ingress.port"] = pa.getName();
                }

                if (!pargs.empty()) {
                    pargs += " ";
                }
                pargs += c;
            }

            svcargs["port"] = pargs;
            svcargs["service.type"] = type;

            const auto svc_name = port.front().getServiceName(name);
            auto service = addChild(svc_name,
                                    Kind::SERVICE, labels, svcargs, "before");

            if (auto ip = getArg("ingress.paths"); ip && !ip->empty()) {
                if (!do_ingress) {
                    LOG_DEBUG << logName() << "Skipping ingress for service type " << type;
                    break;
                }

                if (did_ingress) {
                    LOG_DEBUG << logName() << "Skipping ingress for service type " << type
                              << " because it's already added";
                    break;
                }

                // Add ingress
                conf_t iargs;
                if (!tlsSecretName.empty()) {
                    // Explicit set "ingress.secret" will override below
                    iargs["ingress.secret"] = tlsSecretName;
                }
                for(const auto& [k, v] : svcargs) {
                    static const array<string, 4> relevant = {"ingress.secret", "ingress.paths",
                                                              "port", "ingress.port"};
                    if (find(relevant.begin(), relevant.end(), k) != relevant.end()) {
                        iargs[k] = v;
                    }
                }

                service->addChild(svc_name + "-ingr", Kind::INGRESS, labels, iargs);
                did_ingress = true; // We can only add ingress to one service.
            }
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
        volume.configMap.emplace();
        volume.configMap->name = cf->configmap.metadata.name;

        for(auto& [k, _] : cf->configmap.binaryData) {
            k8api::KeyToPath ktp;
            ktp.key = k;
            ktp.path = k;
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
            if (Engine::mode() == Engine::Mode::DEPLOY) {
                    for(const auto& cond : data.status->conditions) {
                        if (cond.type == "Available" && cond.status == "True") {
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


} // ns
