
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
        if (!secret) {
            secret.emplace();
        }

        if (secret->metadata.name.empty()) {
            secret->metadata.name = name;
        }

        if (secret->metadata.namespace_.empty()) {
            secret->metadata.namespace_ = getNamespace();
        }

        enum class Type {
            UNKNOWN,
            TLS,
            DHCRED
        };

        // Try to determine type from name
        Type type = Type::UNKNOWN;
        if (boost::algorithm::ends_with(name, "-tls")) {
            type = Type::TLS;
        } else if (boost::algorithm::ends_with(name, "-dhcreds")) {
            type = Type::DHCRED;
        }

        if (auto dockerSecret = getArg("imagePullSecrets.fromDockerLogin")
                ; dockerSecret && (type == Type::DHCRED || type == Type::UNKNOWN)) {
            fromFile_ = *dockerSecret;
            if (!filesystem::is_regular_file(fromFile_)) {
                LOG_ERROR << logName() << "Not a file: " << fromFile_;
                throw runtime_error("Missing docker login creds file");
            }

            LOG_TRACE << logName() << " Docker login file: " << fromFile_;

            secret->data[".dockerconfigjson"] = Base64Encode(slurp(fromFile_));
            secret->type = "kubernetes.io/dockerconfigjson";

        } else if (auto tlsSecret = getArgAsKv(getArg("tlsSecret", {}))
                   ; !tlsSecret.empty() && (type == Type::TLS || type == Type::UNKNOWN)) {
            if (tlsSecret.find("key") == tlsSecret.end()) {
                LOG_ERROR << "tlsSecret: Missing `key=` entry for secret " << name;
                throw runtime_error("Missing data for secret");
            }

            if (tlsSecret.find("crt") == tlsSecret.end()) {
                LOG_ERROR << "tlsSecret: Missing `crt=` entry for secret " << name;
                throw runtime_error("Missing data for secret");
            }

            secret->data["tls.key"] = Base64Encode(slurp(tlsSecret.at("key")));
            secret->data["tls.crt"] = Base64Encode(slurp(tlsSecret.at("crt")));
            secret->type = "kubernetes.io/tls";
        } else if (secret->type.empty()){
            LOG_ERROR << "No data given for secret: " << name;
            throw runtime_error("Missing data for secret");
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
            + getNamespace()
            + "/secrets";

    client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Sending Secret "
                  << secret->metadata.name;

        assert(secret);
        LOG_TRACE << "Payload: " << toJson(*secret);

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
                setIsDone();
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
