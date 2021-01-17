#pragma once

#include <atomic>

#include "restc-cpp/restc-cpp.h"

#include <map>
#include "k8deployer/PortForward.h"
#include "k8deployer/Config.h"
//#include "k8deployer/Component.h"
#include "k8deployer/Storage.h"

namespace k8deployer {

class Component;
struct ComponentDataDef;

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
    Cluster(const Config& cfg, const std::string& arg, restc_cpp::RestClient& client,
            const ComponentDataDef& def);

    ~Cluster();

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

    std::future<void> prepare();
    std::future<void> execute();

    bool isExecuting() const noexcept {
        return state_ == State::EXECUTING;
    }

    Storage *getStorage() {
        return storage_.get();
    }

private:
    using action_fn_t = std::function<std::future<void>()>;
    void startEventsLoop();
    void createComponents();
    void setCmds();
    void parseArgs(const std::string& args);
    std::pair<std::string, std::string> split(const std::string& str, char ch) const;

    State state_{State::INIT};
    std::string name_;
    std::unique_ptr<PortForward> portFwd_;
    vars_t variables_;
    restc_cpp::RestClient& client_;
    std::string kubeconfig_; // Empty for default (no arguments)
    const Config& cfg_;
    std::shared_ptr<Component> rootComponent_;
    const ComponentDataDef& dataDef_;

    action_fn_t executeCmd_;
    action_fn_t prepareCmd_;
    std::unique_ptr<Storage> storage_;
};


} // ns
