#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/ConfigMapComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {


ConfigMapComponent::ConfigMapComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
    kind_ = Kind::CONFIGMAP;
    parentRelation_ = ParentRelation::BEFORE;
}


void ConfigMapComponent::prepareDeploy()
{
    if (!prepared_) {
        if (configmap.metadata.name.empty()) {
            configmap.metadata.name = name;
        }

        if (configmap.metadata.namespace_.empty()) {
            configmap.metadata.namespace_ = Engine::config().ns;
        }

        if (auto fileNames = getArg("config.fromFile")) {
            vector<string> files;
            boost::split(files, *fileNames, boost::is_any_of(","));
            for(const auto fileName : files) {
                if (fileName.empty()) {
                    continue;
                }

                LOG_TRACE << logName() << " config-file: " << fileName;
                if (filesystem::is_regular_file(fileName)) {
                    const auto key = filesystem::path{fileName}.filename();
                    configmap.binaryData[key] = Base64Encode(slurp(fileName));
                } else {
                    LOG_ERROR << logName() << "Cannot read config-file : " << fileName;
                    throw runtime_error("Missing file "s + fileName);
                }
            }
        }
        prepared_ = true;
    }

    // Apply recursively
    Component::prepareDeploy();
}

void ConfigMapComponent::addDeploymentTasks(Component::tasks_t &tasks)
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

void ConfigMapComponent::addRemovementTasks(Component::tasks_t &tasks)
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

void ConfigMapComponent::doDeploy(std::weak_ptr<Component::Task> task)
{
    auto url = cluster_->getUrl()
            + "/api/v1/namespaces/"
            + configmap.metadata.namespace_
            + "/configmaps";

    client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Sending ConfigMap "
                  << configmap.metadata.name;

        LOG_TRACE << "Payload: " << toJson(configmap);

        try {
            auto reply = RequestBuilder{ctx}.Post(url)
               .Data(configmap, jsonFieldMappings())
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

void ConfigMapComponent::doRemove(std::weak_ptr<Component::Task> task)
{
    auto url = cluster_->getUrl()
            + "/api/v1/namespaces/"
            + configmap.metadata.namespace_
            + "/configmaps/" + name;

    client().Process([this, url, task](Context& ctx) {

        LOG_DEBUG << logName()
                  << "Deleting ConfigMap "
                  << name;

        try {
            auto reply = RequestBuilder{ctx}.Delete(url)
               .Execute();

            LOG_DEBUG << logName()
                  << "Deleting gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            // We don't get any event's related to this, so just update the states.
            if (auto taskInstance = task.lock()) {
                taskInstance->setState(Task::TaskState::DONE);
            }

            if (state_ == State::RUNNING) {
                setState(State::DONE);
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
