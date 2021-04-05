#include <filesystem>

#include <boost/fusion/adapted.hpp>
#include <boost/process.hpp>
#include <boost/process/args.hpp>

#include "restc-cpp/SerializeJson.h"

#include "k8deployer/logging.h"
#include "include/k8deployer/Engine.h"
#include "k8deployer/Component.h"

using namespace std;
using namespace chrono_literals;

namespace k8deployer {

Engine *Engine::instance_;

Engine::Engine(const Config &config)
    : cfg_{config}
{
//    restc_cpp::Request::Properties properties;
//    properties.cacheMaxConnectionsPerEndpoint = 32;
//    client_ = restc_cpp::RestClient::Create(properties);

    if (cfg_.command == "deploy") {
        mode_ = Mode::DEPLOY;
    } else if (cfg_.command == "delete") {
        mode_ = Mode::DELETE;
    } else if (cfg_.command == "depends") {
        mode_ = Mode::SHOW_DEPENDENCIES;
    } else {
        LOG_ERROR << "Unknown command: " << cfg_.command ;
        throw runtime_error("Unknown command "s + cfg_.command);
    }

    assert(instance_ == nullptr);
    instance_ = this;
}

void Engine::run()
{
    // Create cluster instances
    for(auto& k: cfg_.kubeconfigs) {
        clusters_.emplace_back(make_unique<Cluster>(cfg_, k, clusters_.size()));
    }

    //startPortForwardig();

    std::deque<future<void>> futures;

    for(auto& cluster : clusters_) {
        futures.push_back(cluster->prepare());
    }

    for(auto& f : futures) {
        // TODO: Catch exceptions and deal with setup errore before we start doing something
        f.get();
    }

//    // Let the events backlog be safely ignored...
//    // TODO: Use the futures above to hold the clusters back until the event backlog is received
//    this_thread::sleep_for(5s);
    futures.clear();

    for(auto& cluster : clusters_) {
        futures.push_back(cluster->execute());
    }

    for(auto& f : futures) {
        // TODO: Catch exceptions and deal with errors, potentially, try to roll back
        f.get();
    }

    for(auto& cluster : clusters_) {
        futures.push_back(cluster->pendingWork());
    }

    for(auto& f : futures) {
        // TODO: Catch exceptions and deal with errors, potentially, try to roll back
        f.get();
    }

    LOG_INFO << "Done. Shutting down background threads and async IO.";
    for(auto& cluster : clusters_) {
        try {
            cluster->client().GetIoService().stop();
        } catch (const exception& ex) {
            LOG_WARN << "Caugtht exception from asio shutdown: " << ex.what();
        }
    }

    for(auto& cluster : clusters_) {
        cluster->client().CloseWhenReady();
      }
}

Engine &Engine::instance() noexcept
{
    assert(instance_);
    return *instance_;
}

Cluster *Engine::getCluster(size_t ix)
{
    if (clusters_.size() <= ix) {
        return {};
    }

    return clusters_.at(ix).get();
}

std::tuple<bool, size_t, string> Engine::parseClusterVar(const string &name)
{
  static const regex clusterName{R"(cluster(\d+)\:([a-zA-Z0-9\-\._]+))"};
  smatch tokens;
  if (regex_match(name, tokens, clusterName)) {
      assert(tokens.size() == 3);

      const auto ix = tokens[1].str();
      size_t clusterIx = stoul(ix);
      const auto varName = tokens[2].str();

      return {true, clusterIx, varName};
    }

  return {false, 0, name};
}

string Engine::getClusterVar(size_t clusterIx, const string &varName)
{
    if (auto c = getCluster(clusterIx)) {
        c->getVarsReadyStage().wait();
        if (auto v = c->getVar(varName)) {
            return *v;
        }
    }

    LOG_WARN << "Could not find cluster or variable: cluster" << clusterIx << ':' << varName;
    throw runtime_error{"No such cluster or var: "};
}

void Engine::startPortForwardig()
{
//    for(auto& cluster : clusters_) {
//        //cluster->startProxy();
//    }

//    bool success = true;
//    for(auto& cluster : clusters_) {
//        if (!cluster->waitForProxyToStart()) {
//            LOG_ERROR << "Failed to start proxying to cluster " << cluster->name();
//            success = false;
//        }
//    }

//    if (!success) {
//        throw runtime_error("Failed to start proxying");
//    }
}
}
