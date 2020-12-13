
#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/DeploymentComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

std::future<void> DeploymentComponent::prepareDeploy()
{
    deployment.metadata.name = name;

    auto selector = getSelector();

    deployment.metadata.labels.try_emplace(selector.first, selector.second);
    deployment.spec.selector.matchLabels.try_emplace(selector.first, selector.second);

    if (deployment.spec.template_.metadata.name.empty()) {
        deployment.spec.template_.metadata.name = name;
    }
    deployment.spec.template_.metadata.labels.try_emplace(selector.first, selector.second);

    if (auto replicas = getArg("replicas")) {
        deployment.spec.replicas = stoull(*replicas);
    }

    deployment.spec.selector.matchLabels.try_emplace(selector.first, selector.second);
    deployment.spec.template_.metadata.labels.try_emplace(selector.first, selector.second);

    if (deployment.spec.template_.spec.containers.empty()) {

        k8api::Container container;
        container.name = name;
        container.image = getArg("image", name);

        if (auto port = getArg("port")) {
            k8api::ContainerPort p;
            p.containerPort = static_cast<uint16_t>(stoul(*port));
            p.name = "default";
            if (auto v = getArg("protocol")) {
                p.protocol = *v;
            }
            container.ports.emplace_back(p);
        }

        deployment.spec.template_.spec.containers.push_back(move(container));
    }

    buildDependencies();

    // Apply recursively
    Component::prepareDeploy();

    return dummyReturnFuture();
}

void DeploymentComponent::addTasks(Component::tasks_t& tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            doDeploy();
            task.setState(Task::TaskState::WAITING);
        }

        // Monitoring?
        if (task.isMonitoring() && event) {
            LOG_TRACE << task.component().logName() << " Task " << task.name()
                      << " evaluating event: " << event->message;

            auto key = name + "-";
            if (event->kind == "pod"
                //&& event->metadata._namespace ==
                && event->metadata.name.substr(0, key.size()) == key
                && event->reason == "Created") {

                // A pod with our name was created.
                // TODO: Use this as a hint and query the status of running pods.
                // In order to continue, all our pods should be in avaiable state

                LOG_DEBUG << "A pod with my name was created. I am assuming it relates to me";
                if (++podsStarted_ >= deployment.spec.replicas) {
                    setState(State::DONE);
                }
            }
        }

        task.evaluate();
    });

    tasks.push_back(task);
}


void DeploymentComponent::buildDependencies()
{
    if (labels.empty()) {
        labels["app"] = name; // Use the name as selector
    }

    // A deployment normally needs a service
    const auto serviceEnabled = getBoolArg("service.enabled");
    if (!hasKindAsChild(Kind::SERVICE) && (serviceEnabled && *serviceEnabled)) {
        LOG_DEBUG << logName() << "Adding Service.";
        addChild(name + "-svc", Kind::SERVICE, labels);
    }
}

void DeploymentComponent::doDeploy()
{
    auto url = cluster_->getUrl()
            + "/apis/apps/v1/namespaces/"
            + Engine::config().ns
            + "/deployments";

    Engine::client().Process([this, url](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Sending Deployment "
                  << deployment.metadata.name;

        LOG_TRACE << "Payload: " << toJson(deployment);

        try {
            JsonFieldMapping mappings;
            mappings.entries.push_back({"template_", "template"});
            mappings.entries.push_back({"operator_", "operator"});
            auto reply = RequestBuilder{ctx}.Post(url)
               .Data(deployment, &mappings)
               .Execute();

            LOG_DEBUG << logName()
                  << "Deployment gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;
            return;
        } catch(const RequestFailedWithErrorException& err) {
            LOG_WARN << logName()
                     << "Request failed: " << err.http_response.status_code
                     << ' ' << err.http_response.reason_phrase
                     << ": " << err.what();
        } catch(const std::exception& ex) {
            LOG_WARN << logName()
                     << "Request failed: " << ex.what();
        }

        // TODO: Deal with errors

    });
}


}