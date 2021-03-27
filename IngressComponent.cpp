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

            if (!ip.backend && !parent->service.spec.ports.empty()) {
                ip.backend.emplace();
                ip.backend->setServiceName(ingress.apiVersion, parent->name);

                if (auto portName = getArg("ingress.port")) {
                    LOG_TRACE << logName() << "Using port from 'ingress.port' setting: " << *portName;
                    ip.backend->setServicePortName(ingress.apiVersion, *portName);
                    ip.backend->servicePort = *portName;
                } else {
                    LOG_TRACE << logName() << "Using first port from parent: "
                              << parent->service.spec.ports.front().name;
                    ip.backend->setServicePortName(ingress.apiVersion, parent->service.spec.ports.front().name);
                    ip.backend->servicePort = parent->service.spec.ports.front().name;
                }
            }

            if (ip.backend) {
                LOG_TRACE << logName()
                          << "Adding ingress path " << ip.path << " [" << ip.pathType
                          << "] to service " << ip.backend->getServiceName(ingress.apiVersion)
                          << ":" << ip.backend->getServicePortName(ingress.apiVersion);

                ir.http->paths.emplace_back(move(ip));
            } else {
                LOG_WARN << logName() << "No backend for ingress!";
            }
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
            }, [wself=weak_from_this()] (const auto& data) {
                // TODO: It must be allowed for load-balancers to not come online,
                //       but if we know that we use a load-balancer, then we must wait.
                if (data.status) {
                    if (!data.status->loadBalancer.ingress.empty()) {
                        if (auto self = wself.lock()) {
                          dynamic_cast<IngressComponent&>(*self.get()).loadBalancerIp_ = data.status->loadBalancer.ingress.front().ip;
                          return true;
                        }
                    }
                }

                return Engine::config().useLoadBalancerIp == false;
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

    if (cluster_->getDns()) {
        auto dnsTask = make_shared<Task>(*this, name + "-provision-dns",
                                         [&](Task& task, const k8api::Event */*event*/) {
            // Execution?
            if (task.state() == Task::TaskState::READY) {
                task.setState(Task::TaskState::EXECUTING);

               if (auto dns = cluster_->getDns()) {
                  for(const auto& rule : ingress.spec->rules) {
                      if (!rule.host.empty()) {
                          auto ip = loadBalancerIp_;
                          if (ip.empty()) {
                              if (auto var = cluster_->getVar("clusterIp")) {
                                  ip = *var;
                              }
                          }
                          if (ip.empty()) {
                            LOG_WARN << logName() << "Don't know IP for hostname " << rule.host
                                     << ". Cannot provision entry in DNS server";
                          } else try {
                              LOG_DEBUG << logName() << "Provisioning dns entry for "
                                        << rule.host << " to IP " << ip;
                              dns->provisionHostname(rule.host, {ip}, {},
                                                     [w = task.weak_from_this()](bool success) {
                                  if (auto task = w.lock()) {
                                      task->setState(success
                                                     ? Task::TaskState::DONE
                                                     : Task::TaskState::FAILED);
                                  }
                              });
                          } catch(const exception& ex) {
                              LOG_WARN << logName() << "Failed to provision DNS name for " <<  rule.host;
                              task.setState(Task::TaskState::FAILED);
                          }
                      }
                  }
                } else {
                    LOG_WARN << logName() << "The DNS config vanished... Cannot provision hostnames.";
                    task.setState(Task::TaskState::FAILED);
                }
            }

            task.evaluate();
        });

        dnsTask->addDependency(task);
        tasks.push_back(dnsTask);
    }


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

    if (cluster_->getDns()) {
        auto dnsTask = make_shared<Task>(*this, name + "-provision-dns",
                                         [&](Task& task, const k8api::Event */*event*/) {

            // Execution?
            if (task.state() == Task::TaskState::READY) {
                task.setState(Task::TaskState::EXECUTING);

                if (auto dns = cluster_->getDns()) {
                    for(const auto& rule : ingress.spec->rules) {
                        if (!rule.host.empty()) {
                            try {
                                LOG_DEBUG << logName() << "Removing dns entry for " << rule.host;
                                dns->deleteHostname(rule.host, [w = task.weak_from_this()](bool success) {
                                    ; // just ignore for now
                                });
                            } catch(const exception& ex) {
                                LOG_WARN << logName() << " Failed to delete DNS name for " <<  rule.host;
                            }
                        }
                    }
                }

                task.setState(Task::TaskState::DONE);
            }

            task.evaluate();
        });

        tasks.push_back(dnsTask);
    }


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
