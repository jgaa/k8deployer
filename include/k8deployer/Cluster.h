#pragma once

#include "restc-cpp/restc-cpp.h"

#include <map>
#include "k8deployer/PortForward.h"
#include "k8deployer/Config.h"
#include "k8deployer/Component.h"

namespace k8deployer {

class Cluster
{
public:
    enum class State {
        INIT,
        EXECUTING,
        SHUTDOWN,
        DONE,
        ERROR
    };

    using vars_t = std::map<std::string, std::string>;
    Cluster(const Config& cfg, const std::string& arg, restc_cpp::RestClient& client);

    void run();

    const std::string& name() const noexcept {
        return name_;
    }

    restc_cpp::RestClient& client() noexcept {
        return client_;
    }

    void startProxy();
    bool waitForProxyToStart() {
        return portFwd_->waitForStarted();
    }

    std::string getVars() const;
    void setState(State state) {
        state_ = state;
    }
    State state() const noexcept {
        return state_;
    }

    // Get the URL to the api server in the cluster
    std::string getUrl() const;

private:
    void deploy();
    void startEventsLoop();
    void createComponents();
    void parseArgs(const std::string& args);
    std::pair<std::string, std::string> split(const std::string& str, char ch) const;

    State state_ = State::INIT;
    std::string name_;
    std::unique_ptr<PortForward> portFwd_;
    vars_t variables_;
    restc_cpp::RestClient& client_;
    std::string kubeconfig_; // Empty for default (no arguments)
    const Config& cfg_;
    std::unique_ptr<Component> rootComponent_;
};


} // ns
