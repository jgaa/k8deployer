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
    enum class Mode {
        DEPLOY,
        DELETE,
        SHOW_DEPENDENCIES
    };

    Engine(const Config& config);

    ~Engine() {
        instance_ = nullptr;
    }

    void run();

    static Engine& instance() noexcept;

    static const Config& config() noexcept {
        return instance().cfg_;
    }

    static Mode mode() noexcept {
        return instance().mode_;
    }

    Cluster *getCluster(size_t ix);

    static std::tuple<bool, size_t, std::string> parseClusterVar(const std::string& name);

    std::string getClusterVar(size_t clusterIx, const std::string& varName);

private:
    void startPortForwardig();

    //std::unique_ptr<restc_cpp::RestClient> client_;
    std::vector<std::unique_ptr<Cluster>> clusters_;
    const Config cfg_;
    static Engine *instance_;
    Mode mode_ = Mode::DEPLOY;
};

} // ns
