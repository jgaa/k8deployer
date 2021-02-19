#include "include/k8deployer/IngressComponent.h"

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/IngressComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Component.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

void IngressComponent::prepareDeploy()
{
    if (configmap.metadata.name.empty()) {
        ingress.metadata.name = name;
    }

    if (configmap.metadata.namespace_.empty()) {
        ingress.metadata.namespace_ = getNamespace();
    }

    if (Engine::config().useNetworkingBetaV1) {
        ingress.apiVersion = "networking.k8s.io/v1beta1";
    }

    if (auto defs = getArg("ingress.paths")) {

        // Format [hostname:]/path[...][/*][ new-path-definition]...
        // If a path ends with "/*" pathType=Prefix, else pathType=Exact

        const auto parent = parentPtr();
        if (!parent || parent->getKind() != Kind::SERVICE) {
            LOG_ERROR << "A ingress needs to have a service as a parent.";
            throw runtime_error("No parent / parent not a service");
        }

        // See if we have a hostname
        string host;
        bool haveHost = false;
        locale loc{"C"};
        for(const auto ch : *defs) {
            if (ch == ':') {
                if (!host.empty()) {
                    haveHost = true;
                }
            }

            if (isalpha(ch, loc) || isdigit(ch, loc) || ch == '.' || ch == '_' || ch == '-') {
                host += ch;
                continue;
            }

            break;
        }

        k8api::IngressRule ir;
        const string pathsSegment = haveHost ? defs.value().substr(host.size() + 1) : *defs;
        if (haveHost) {
            ir.host = host;
        }
        auto paths = getArgAsStringList(pathsSegment);
        for(const auto& path: paths) {
            k8api::HTTPIngressPath ip;
            if (path.size() >= 2
                    && path[path.size() -2] == '/'
                    && path[path.size() -1] == '*') {

                if (path.size() > 2) {
                    ip.path = path.substr(0, path.size() - 2);
                } else {
                    ip.path = "/";
                }
                ip.pathType = "Prefix";
            } else {
                ip.path = path;
                ip.pathType = "Exact";
            }

            if (!ir.http) {
                ir.http.emplace();
            }

            // TODO: Add some means to override the port to a path, so we can
            //       support services with multiple ports, and not just pick the
            //       first port.
            if (!ip.backend && !parent->service.spec.ports.empty()) {
                ip.backend.emplace();
                ip.backend->setServiceName(ingress.apiVersion, parent->name);
                ip.backend->setServicePortName(ingress.apiVersion, parent->service.spec.ports.front().name);
                ip.backend->servicePort = parent->service.spec.ports.front().name;
            }

            LOG_TRACE << logName()
                      << "Adding ingress path " << ip.path << " [" << ip.pathType
                      << "] to service " << ip.backend->getServiceName(ingress.apiVersion)
                      << ":" << ip.backend->getServicePortName(ingress.apiVersion);

            ir.http->paths.emplace_back(move(ip));
        }

        if (!ingress.spec) {
            ingress.spec.emplace();
        }

        ingress.spec->rules.emplace_back(move(ir));
    }

    if (auto secret = getArg("ingress.secret")) {
        k8api::IngressTLS tls;

        // TODO: Add parsing for "hostname=secret" and also add hostname if specified.
        tls.secretName = *secret;
        ingress.spec->tls.emplace_back(move(tls));
    }

    // Apply recursively
    Component::prepareDeploy();
}

bool IngressComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        sendProbe<k8api::Ingress>(*this, getAccessUrl(),
            [wself=weak_from_this(), fn=move(fn)]
            (const std::optional<k8api::Ingress>& /*object*/, K8ObjectState state) {
                if (auto self = wself.lock()) {
                    assert(fn);
                    fn(state);
                }
            }, [] (const auto& data) {
                // TODO: It must be allowed for load-balancers to not come online,
                //       but if we know that we use a load-balancer, then we must wait.
                return true;
            }
        );
    }

    return true;

}

void IngressComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void IngressComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void IngressComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    if (auto t = task.lock()) {
        t->startProbeAfterApply = true;
    }
    sendApply(ingress, getCreationUrl(), task);
}

void IngressComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    sendDelete(getAccessUrl(), task, true);
}

string IngressComponent::getCreationUrl() const
{
    const auto url = cluster_->getUrl()
            + "/apis/" + ingress.apiVersion + "/namespaces/"
            + getNamespace() + "/ingresses";
    return url;
}



} // ns
