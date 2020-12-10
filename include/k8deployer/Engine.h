#pragma once

#include "restc-cpp/restc-cpp.h"
#include "k8deployer/Config.h"
#include "k8deployer/Cluster.h"

namespace k8deployer {

class Engine
{
public:
    Engine(const Config& config)
        : client_{restc_cpp::RestClient::Create()}, cfg_{config}
    {
        assert(instance_ == nullptr);
        instance_ = this;
    }

    ~Engine() {
        instance_ = nullptr;
    }

    void run();

    static Engine& instance() noexcept {
        assert(instance_);
        return *instance_;
    }

    static const Config& config() noexcept {
        return instance().cfg_;
    }

private:
    void startPortForwardig();

    std::unique_ptr<restc_cpp::RestClient> client_;
    std::vector<std::unique_ptr<Cluster>> clusters_;
    const Config cfg_;
    static Engine *instance_;
};

} // ns
