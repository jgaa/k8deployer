#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include "restc-cpp/restc-cpp.h"

#include <map>
#include "k8deployer/Config.h"
#include "k8deployer/Storage.h"
#include "k8deployer/DataDef.h"
#include "k8deployer/Kubeconfig.h"
#include "k8deployer/DnsProvisioner.h"

namespace k8deployer {

class Component;

class Cluster
{
public:
    enum class State {
        INIT,
        EXECUTING,
        SHUTDOWN,
        LOGGING,
        DONE,
        ERROR
    };

    using vars_t = std::map<std::string, std::string>;
    Cluster(const Config& cfg, const std::string& arg, const size_t id);

    ~Cluster();

    const std::string& name() const noexcept {
        return name_;
    }

    restc_cpp::RestClient& client() noexcept {
        assert(client_);
        return *client_;
    }

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
    std::future<void> pendingWork();

    bool isExecuting() const noexcept {
        return state_ == State::EXECUTING;
    }

    Storage *getStorage() {
        return storage_.get();
    }

    std::shared_future<void> getVarsReadyStage() {
        return vars_ready_;
    }

    std::shared_future<void> getDefinitionsReady() {
        return definitions_ready_;
    }

    std::shared_future<void> getBasicComponentsReady() {
        return basic_components_ready_;
    }

    std::shared_future<void> getPreparedReady() {
        return prepared_ready_;
    }

    auto& getIoService() {
      assert(client_);
      return client_->GetIoService();
    }

    // Returns false if there is no such component
    bool addStateListener(const std::string& componentName,
                          const std::function<void (const Component& component)>& fn);

    void add(Component *component);

    Component *getComponent(const std::string& name);

    DnsProvisioner *getDns() {
        return dns_.get();
    }

    void listenForContainers();

    Component& getRootComponent() {
        assert(rootComponent_);
        return *rootComponent_;
    }

private:
    using action_fn_t = std::function<std::future<void>()>;
    void loadKubeconfig();
    void startEventsLoop();
    void readDefinitions();
    void createComponents();
    void setCmds();
    void parseArgs(const std::string& args);
    void prepareLogging(const k8api::Pod& pod, const k8api::ContainerStatus& container);
    void startLogging(const k8api::Pod& pod, const k8api::ContainerStatus& container);
    void stopLogging(const k8api::Pod& pod, const k8api::ContainerStatus& container);
    std::filesystem::path logPath(const k8api::Pod& pod, const k8api::ContainerStatus& container);
    std::pair<std::string, std::string> split(const std::string& str, char ch) const;

    State state_{State::INIT};
    std::string url_;
    std::string name_;
    vars_t variables_;
    std::unique_ptr<DnsProvisioner> dns_;
    std::promise<void> pendingWork_;
    std::map<std::string, Component *> components_;
    std::shared_ptr<Component> rootComponent_;
    std::string kubeconfig_; // Empty for default (no arguments)
    const Config& cfg_;

    action_fn_t executeCmd_;
    action_fn_t prepareCmd_;
    std::unique_ptr<Storage> storage_;
    std::unique_ptr<ComponentDataDef> dataDef_;
    std::string verb_ = "Executing";

    std::promise<void> vars_ready_pr_;
    std::promise<void> definitions_ready_pr_;
    std::promise<void> basic_components_pr_;
    std::promise<void> prepared_ready_pr_;

    std::shared_future<void> vars_ready_{vars_ready_pr_.get_future()};
    std::shared_future<void> definitions_ready_{definitions_ready_pr_.get_future()};
    std::shared_future<void> basic_components_ready_{basic_components_pr_.get_future()};
    std::shared_future<void> prepared_ready_{prepared_ready_pr_.get_future()};
    std::mutex mutex_;
    std::map<std::string /* container id */, k8api::ContainerStatus /* previous known state*/> knownContainers_;
    std::map<std::string /* container id */, std::string /* path */> openLogs_;
    std::shared_ptr<restc_cpp::RestClient> client_;
};


} // ns
