#pragma once

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/Engine.h"
#include "k8deployer/Component.h"
#include "k8deployer/logging.h"

namespace k8deployer {

class BaseComponent : public Component
{
public:
    BaseComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {}

    virtual k8api::ObjectMeta *getMetadata() = 0;
    virtual k8api::PodTemplateSpec *getPodTemplate() = 0;
    virtual k8api::LabelSelector *getLabelSelector() = 0;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void addRemovementTasks(tasks_t &tasks) override;

    virtual size_t getReplicas() const {
        return 1;
    }

    void basicPrepareDeploy();
    virtual void buildInitContainers();

    template <typename T>
    void sendApply(const T& data, const std::string& url, std::weak_ptr<Task> task)
    {
        Engine::client().Process([this, url, task, &data](auto& ctx) {

            LOG_DEBUG << logName() << "Applying payload";
            LOG_TRACE << logName() << "Payload: " << toJson(data);

            try {
                auto reply = restc_cpp::RequestBuilder{ctx}.Post(url)
                   .Data(data, jsonFieldMappings())
                   .Execute();

                LOG_DEBUG << logName()
                      << "Applying gave response: "
                      << reply->GetResponseCode() << ' '
                      << reply->GetHttpResponse().reason_phrase;
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
                taskInstance->setState(Task::TaskState::FAILED);
            }
            setState(State::FAILED);

        });
    }

    void sendDelete(const std::string& url, std::weak_ptr<Component::Task> task,
                    bool ignoreErrors = false);

    virtual void doDeploy(std::weak_ptr<Task> task) = 0;
    virtual void doRemove(std::weak_ptr<Task> task) = 0;

    size_t podsStarted_ = 0;
};

} // ns

class BaseComponent
{
public:
    BaseComponent();
};

