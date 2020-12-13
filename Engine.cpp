
#include "k8deployer/logging.h"
#include "include/k8deployer/Engine.h"

using namespace std;

namespace k8deployer {

Engine *Engine::instance_;

void Engine::run()
{
    // Create cluster instances
    for(auto& k: cfg_.kubeconfigs) {
        clusters_.emplace_back(make_unique<Cluster>(cfg_, k, *client_));
    }

    startPortForwardig();

    std::deque<future<void>> futures;

    for(auto& cluster : clusters_) {
        futures.push_back(cluster->prepare());
    }

    for(auto& f : futures) {
        // TODO: Catch exceptions and deal with setup errore before we start doing something
        f.get();
    }

    futures.clear();

    for(auto& cluster : clusters_) {
        futures.push_back(cluster->execute());
    }

    for(auto& f : futures) {
        // TODO: Catch exceptions and deal with errors, potentially, try to roll back
        f.get();
    }

    LOG_INFO << "Done. Shutting down proxies.";
    try {
        client_->GetIoService().stop();
    } catch (const exception& ex) {
        LOG_WARN << "Caugtht exception from asio shutdown: " << ex.what();
    }

    client_->CloseWhenReady();
}

void Engine::startPortForwardig()
{
    for(auto& cluster : clusters_) {
        cluster->startProxy();
    }

    bool success = true;
    for(auto& cluster : clusters_) {
        if (!cluster->waitForProxyToStart()) {
            LOG_ERROR << "Failed to start proxying to cluster " << cluster->name();
            success = false;
        }
    }

    if (!success) {
        throw runtime_error("Failed to start proxying");
    }
}

}
