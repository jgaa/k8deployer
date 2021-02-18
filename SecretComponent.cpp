
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/SecretComponent.h"
#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {


SecretComponent::SecretComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::SECRET;
    parentRelation_ = ParentRelation::BEFORE;
}


void SecretComponent::prepareDeploy()
{
    if (!prepared_) {
        if (configmap.metadata.name.empty()) {
            configmap.metadata.name = name;
        }

        if (configmap.metadata.namespace_.empty()) {
            configmap.metadata.namespace_ = Engine::config().ns;
        }

        if (auto dockerSecret = getArg("imagePullSecrets.fromDockerLogin")) {
            fromFile_ = *dockerSecret;
            if (!filesystem::is_regular_file(fromFile_)) {
                LOG_ERROR << logName() << "Not a file: " << fromFile_;
                throw runtime_error("Missing docker login creds file");
            }

            LOG_TRACE << logName() << " Docker login file: " << fromFile_;

            secret = k8api::Secret{};
            secret->data["dockerconfigjson"] = Base64Encode(slurp(fromFile_));
            secret->type = "kubernetes.io/dockerconfigjson";

            const auto key = filesystem::path{fromFile_}.filename();
            configmap.binaryData[key] = Base64Encode(slurp(fromFile_));

        }

        prepared_ = true;
    }

    // Apply recursively
    Component::prepareDeploy();
}

void SecretComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void SecretComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void SecretComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    auto url = cluster_->getUrl()
            + "/api/v1/namespaces/"
            + service.metadata.namespace_
            + "/secrets";

    client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Sending Secret "
                  << secret->metadata.name;

        LOG_TRACE << "Payload: " << toJson(secret);

        try {
            auto reply = RequestBuilder{ctx}.Post(url)
               .Data(secret, jsonFieldMappings())
               .Execute();

            LOG_DEBUG << logName()
                  << "Applying gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            // We don't get any event's related to the service, so just update the states.
            if (auto taskInstance = task.lock()) {
                taskInstance->setState(Task::TaskState::DONE);
            }

            if (state_ == State::RUNNING) {
                setState(State::DONE);
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

void SecretComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    auto url = cluster_->getUrl()
            + "/api/v1/namespaces/"
            + secret->metadata.namespace_
            + "/secrets/" + name;

    client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Deleting Secret "
                  << name;

        try {
            auto reply = RequestBuilder{ctx}.Delete(url)
               .Execute();

            LOG_DEBUG << logName()
                  << "Deleting gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            // We don't get any event's related to the service, so just update the states.
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
    });
}

} // ns
