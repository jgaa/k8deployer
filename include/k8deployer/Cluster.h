#pragma once

#include <atomic>

#include "restc-cpp/restc-cpp.h"

#include <map>
#include "k8deployer/PortForward.h"
#include "k8deployer/Config.h"
#include "k8deployer/Storage.h"
#include "k8deployer/DataDef.h"
#include "k8deployer/Kubeconfig.h"

namespace k8deployer {

class Component;

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
    Cluster(const Config& cfg, const std::string& arg);

    ~Cluster();

    const std::string& name() const noexcept {
        return name_;
    }

    restc_cpp::RestClient& client() noexcept {
        assert(client_);
        return *client_;
    }

    //void startProxy();
//    bool waitForProxyToStart() {
//        return portFwd_->waitForStarted();
//    }

    std::string getVars() const;
    void setState(State state) {
        state_ = state;
    }

    std::optional<std::string> getVar(const std::string& key) const;

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
    void loadKubeconfig();
    void startEventsLoop();
    void readDefinitions();
    void createComponents();
    void setCmds();
    void parseArgs(const std::string& args);
    std::pair<std::string, std::string> split(const std::string& str, char ch) const;

    State state_{State::INIT};
    std::string url_;
    std::string name_;
    std::unique_ptr<PortForward> portFwd_;
    vars_t variables_;
    std::shared_ptr<restc_cpp::RestClient> client_;
    std::string kubeconfig_; // Empty for default (no arguments)
    const Config& cfg_;
    std::shared_ptr<Component> rootComponent_;

    action_fn_t executeCmd_;
    action_fn_t prepareCmd_;
    std::unique_ptr<Storage> storage_;
    std::unique_ptr<ComponentDataDef> dataDef_;
    std::string verb_ = "Executing";
};


} // ns
