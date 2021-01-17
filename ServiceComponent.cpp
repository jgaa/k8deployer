#include "k8deployer/ServiceComponent.h"
#include "k8deployer/BaseComponent.h"
#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/probe.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

void ServiceComponent::prepareDeploy()
{
    if (service.metadata.name.empty()) {
        service.metadata.name = name;
    }

    if (service.metadata.namespace_.empty()) {
        service.metadata.namespace_ = Engine::config().ns;
    }

    auto selector = getSelector();

    service.metadata.labels.try_emplace(selector.first, selector.second);
    service.spec.selector.try_emplace(selector.first, selector.second);
    service.spec.type = getArg("service.type", service.spec.type);

    if (service.spec.type.empty()) {
        if (getArg("service.nodePort")) {
           // TODO: Add support for LoadBalancer here?
           service.spec.type = "NodePort";
        }
    }

    if (auto parent = parent_.lock()) {
        if (auto depl = dynamic_cast<BaseComponent *>(parent.get())) {
            auto& containers = depl->getPodTemplate()->spec.containers;

            if (service.spec.ports.empty()) {
                // Try to use the known ports from all the containers in the pod
                size_t cnt = 0;
                for(const auto& container : containers) {
                    for(const auto& dp : container.ports) {
                        ++cnt;
                        k8api::ServicePort sport;
                        sport.protocol = dp.protocol;

                        int extport = dp.hostPort;
                        if (cnt == 1 && extport <= 0) {
                            extport = getIntArg("port", dp.containerPort);
                        }

                        if (extport <= 0) {
                            extport = dp.containerPort;
                        }

                        sport.port = extport;

                        if (!dp.name.empty()) {
                            sport.targetPort = dp.name;
                            sport.name = dp.name;
                        } else {
                            sport.targetPort = dp.hostPort;
                            sport.name = "sport-" + to_string(cnt);
                        }

                        // FIXME: Setting nodePort yields error: 422 Unprocessable Entity
                        if (cnt == 1 && sport.nodePort <= 0) {
                            sport.nodePort = getIntArg("service.nodePort", 0);
                        }

                        LOG_TRACE << logName() << "Added port " << sport.name
                                  << " To service " << service.metadata.name;

                        service.spec.ports.push_back(move(sport));
                    }
                }
            }
        }
    }

    // Apply recursively
    Component::prepareDeploy();
}

bool ServiceComponent::probe(std::function<void (Component::K8ObjectState)> fn)
{
    if (fn) {
        auto url = cluster_->getUrl()
                + "/api/v1/namespaces/"
                + service.metadata.namespace_
                + "/services/" + name;

        sendProbe<k8api::Service>(*this, url, [wself=weak_from_this(), fn=move(fn)]
                              (const std::optional<k8api::Service>& /*object*/, K8ObjectState state) {
            if (auto self = wself.lock()) {
                assert(fn);
                fn(state);
            }
        });
    }

    return true;
}

void ServiceComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void ServiceComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void ServiceComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    auto url = cluster_->getUrl()
            + "/api/v1/namespaces/"
            + service.metadata.namespace_
            + "/services";

    Engine::client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Sending Service "
                  << service.metadata.name;

        LOG_TRACE << "Payload: " << toJson(service);

        try {
            auto reply = RequestBuilder{ctx}.Post(url)
               .Data(service, jsonFieldMappings())
               .Execute();

            LOG_DEBUG << logName()
                  << "Applying gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            if (state_ == State::RUNNING) {
                if (auto taskInstance = task.lock()) {
                    taskInstance->setState(Task::TaskState::WAITING);
                    taskInstance->schedulePoll();
                }
            }

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

        if (auto taskInstance = task.lock()) {
            taskInstance->setState(Task::TaskState::FAILED);
        }

        if (state_ == State::RUNNING) {
            setState(State::FAILED);
        }

    });
}

void ServiceComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    auto url = cluster_->getUrl()
            + "/api/v1/namespaces/"
            + service.metadata.namespace_
            + "/services/" + name;

    Engine::client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Deleting Service "
                  << service.metadata.name;

        try {
            auto reply = RequestBuilder{ctx}.Delete(url)
               .Execute();

            LOG_DEBUG << logName()
                  << "Deletion gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            if (auto taskInstance = task.lock()) {
                taskInstance->setState(Task::TaskState::DONE);
            }
            return;
        } catch(const RequestFailedWithErrorException& err) {
            if (err.http_response.status_code == 404) {
                // Perfectly OK
                if (auto taskInstance = task.lock()) {
                    taskInstance->setState(Task::TaskState::DONE);
                }
                return;
            }

            LOG_WARN << logName()
                     << "Request failed: " << err.http_response.status_code
                     << ' ' << err.http_response.reason_phrase
                     << ": " << err.what();
        } catch(const std::exception& ex) {
            LOG_WARN << logName()
                     << "Request failed: " << ex.what();
        }

        if (auto taskInstance = task.lock()) {
            taskInstance->setState(Task::TaskState::FAILED);
        }

        if (state_ == State::RUNNING) {
            setState(State::FAILED);
        }

    });
}


} // ns
