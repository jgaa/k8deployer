#pragma once

#include <cassert>

#include "restc-cpp/restc-cpp.h"
#include "k8deployer/Config.h"
#include "k8deployer/Cluster.h"

namespace k8deployer {

struct ComponentDataDef;

class Engine
{
public:
    Engine(const Config& config);

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

    static restc_cpp::RestClient& client() noexcept {
        assert(instance().client_);
        return *instance().client_;
    }

private:
    void startPortForwardig();
    void readDefinitions();

    std::unique_ptr<restc_cpp::RestClient> client_;
    std::vector<std::unique_ptr<Cluster>> clusters_;
    const Config cfg_;
    static Engine *instance_;
    std::unique_ptr<ComponentDataDef> dataDef_;
};

} // ns
