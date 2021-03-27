#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class HttpRequestComponent : public Component
{
public:
    HttpRequestComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

    void prepareDeploy() override;

protected:
    void addDeploymentTasks(tasks_t& tasks) override;
    void sendRequest(std::weak_ptr<Task>& wtask);
    restc_cpp::Request::Type toType(const std::string& name);

    restc_cpp::Request::Type rct_ = restc_cpp::Request::Type::GET;
    std::string url_;
    std::string json_;
    std::string message_;
    std::string user_;
    std::string passwd_;
    size_t retries_ = 0;
    size_t retryDelaySeconds_ = 5;
    size_t currentCnt_ = 0;
    std::shared_ptr<restc_cpp::RestClient> client_;
};

}
