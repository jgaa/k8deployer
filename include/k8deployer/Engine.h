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
    {}

    void run();

private:
    void startPortForwardig();

    std::unique_ptr<restc_cpp::RestClient> client_;
    std::vector<std::unique_ptr<Cluster>> clusters_;
    const Config& cfg_;
};

} // ns
