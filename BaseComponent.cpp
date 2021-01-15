
#include "k8deployer/BaseComponent.h"
#include "k8deployer/DeploymentComponent.h"
#include "k8deployer/Cluster.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

// Does the general preparations to prepare a Job, Deoplyment or StatefulSet
void BaseComponent::basicPrepareDeploy()
{
    if (auto meta = getMetadata()) {
        if (meta->name.empty()) {
            meta->name = name;
        }

        if (meta->namespace_.empty()) {
            meta->namespace_ = Engine::config().ns;
        }

        auto selector = getSelector();

        meta->labels.try_emplace(selector.first, selector.second);

        if (auto podTemplate = getPodTemplate()) {
            podTemplate->metadata.labels.try_emplace(selector.first, selector.second);

            if (podTemplate->metadata.name.empty()) {
                podTemplate->metadata.name = name;
            }

            podTemplate->metadata.labels.try_emplace(selector.first, selector.second);

            k8api::Container container;
            container.name = name;
            container.image = getArg("image", name);
            container.args = getArgAsStringList("pod.args", ""s);
            container.env = getArgAsEnvList("pod.env", ""s);
            container.command = getArgAsStringList("pod.command", ""s);

            if (auto ports = getArgAsStringList("port", ""s); !ports.empty()) {
                for(const auto& port: ports) {
                    k8api::ContainerPort p;
                    p.containerPort = static_cast<uint16_t>(stoul(port));
                    p.name = "port-"s + port;
                    if (auto v = getArg("protocol")) {
                        p.protocol = *v;
                    }
                    container.ports.emplace_back(p);
                }
            }

            container.startupProbe = startupProbe;
            container.livenessProbe = livenessProbe;
            container.readinessProbe = readinessProbe;

            podTemplate->spec.containers.push_back(move(container));

            if (auto dhcred = getArg("imagePullSecrets")) {
                // Use existing secret
                if (!dhcred->empty()) {
                    k8api::LocalObjectReference lor = {*dhcred};
                    podTemplate->spec.imagePullSecrets.push_back(lor);
                    LOG_DEBUG << logName() << "Using imagePullSecret " << *dhcred;
                }
            }
        }

        if (auto ls = getLabelSelector()) {
            ls->matchLabels.try_emplace(selector.first, selector.second);
        }
    }
}

void BaseComponent::addDeploymentTasks(Component::tasks_t& tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            buildInitContainers(); // This must be done after all components are initialized
            task.setState(Task::TaskState::EXECUTING);
            doDeploy(task.weak_from_this());
            task.setState(Task::TaskState::WAITING);
        }

        // Monitoring?
        if (task.isMonitoring() && event) {
            LOG_TRACE << task.component().logName() << " Task " << task.name()
                      << " evaluating event: " << event->message;

            auto key = name + "-";
            if (event->involvedObject.kind == "Pod"
                && event->involvedObject.name.substr(0, key.size()) == key
                && event->metadata.namespace_ == deployment.metadata.namespace_
                && event->metadata.name.substr(0, key.size()) == key
                && event->reason == "Created") {

                // A pod with our name was created.
                // TODO: Use this as a hint and query the status of running pods.
                // In order to continue, all our pods should be in avaiable state

                LOG_DEBUG << "A pod with my name was created. I am assuming it relates to me";
                if (++podsStarted_ >= deployment.spec.replicas) {
                    task.setState(Task::TaskState::DONE);
                    setState(State::DONE);
                }
            }
        }

        task.evaluate();
    });

    tasks.push_back(task);
    Component::addDeploymentTasks(tasks);
}

void BaseComponent::addRemovementTasks(Component::tasks_t &tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            doRemove(task.weak_from_this());
            task.setState(Task::TaskState::WAITING);
        }

        // Monitoring?
        if (task.isMonitoring() && event) {
            LOG_TRACE << task.component().logName() << " Task " << task.name()
                      << " evaluating event: " << event->message;

            auto key = name + "-";
            if (event->involvedObject.kind == "Pod"
                && event->involvedObject.name.substr(0, key.size()) == key
                && event->metadata.namespace_ == deployment.metadata.namespace_
                && event->metadata.name.substr(0, key.size()) == key
                && event->reason == "Killing") {

                // A pod with our name was deleted.
                // TODO: Use this as a hint and query the status of running pods.
                // In order to continue, all our pods should be in avaiable state

                LOG_DEBUG << logName() << "A pod with my name was deleted. I am assuming it relates to me";

                if (++podsStarted_ >= getReplicas()) {
                    task.setState(Task::TaskState::DONE);
                    setState(State::DONE);
                }
            }
        }

        task.evaluate();
    });

    tasks.push_back(task);
    Component::addRemovementTasks(tasks);
}

void BaseComponent::buildInitContainers()
{
    // Check dependencies
    if (auto podTemplate = getPodTemplate()) {
        for(const auto& name: depends) {
            // Add dependencies for any service with, or assigned to, that name
            forAllComponents([&](Component& c) {
                Component *target = {};
                if (name == c.name) {
                   if (c.getKind() == Kind::SERVICE) {
                       target = &c;
                   } else if (c.getKind() == Kind::DEPLOYMENT) {
                       for(auto& cc : c.getChildren()) {
                           if (cc->getKind() == Kind::SERVICE) {
                               target = cc.get();
                               break;
                           }
                       }
                   }
                }

                if (target) {
                    k8api::Container init;
                    init.command = {"sh"s,
                                    "-c"s,
                                    "until nslookup "s + target->name + "; "
                                    + " do echo waiting for "s + target->name
                                    + "; sleep 2; done;"};
                    init.image = "busybox";
                    init.name = "init-"s + c.name + "-" + target->name;

                    LOG_DEBUG << c.logName()
                              << "Adding dependency to " << target->logName()
                              << "initContainer " << init.name;

                    podTemplate->spec.initContainers.push_back(move(init));
                    dependsOn_.push_back(target);
                }
            });
        }
    }
}

void BaseComponent::sendDelete(const string &url, std::weak_ptr<Component::Task> task,
                               bool ignoreErrors)
{
    Engine::client().Process([this, url, task, ignoreErrors](auto& ctx) {

        try {
            auto reply = restc_cpp::RequestBuilder{ctx}.Delete(url)
               .Execute();

            LOG_DEBUG << logName()
                  << "Delete gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            // We don't get any event's related to deleting the deployment, so just update the states.
            if (auto taskInstance = task.lock()) {
                taskInstance->setState(Task::TaskState::DONE);
            }
            return;
        } catch(const restc_cpp::RequestFailedWithErrorException& err) {
            LOG_WARN << logName()
                     << "Request failed: " << err.http_response.status_code
                     << ' ' << err.http_response.reason_phrase
                     << ": " << err.what();

        } catch(const std::exception& ex) {
            LOG_WARN << logName()
                     << "Request failed: " << ex.what();
        }

        if (auto taskInstance = task.lock()) {
            taskInstance->setState(ignoreErrors ? Task::TaskState::DONE : Task::TaskState::FAILED);
        }

        if (!ignoreErrors) {
            setState(State::FAILED);
        }
    });
}

}