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
    void sendRequest();
    restc_cpp::Request::Type toType(const std::string& name);

    restc_cpp::Request::Type rct_ = restc_cpp::Request::Type::GET;
    std::string url_;
    std::string json_;
    std::string user_;
    std::string passwd_;
};

}
