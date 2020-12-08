
#include "k8deployer/logging.h"
#include "include/k8deployer/Engine.h"

using namespace std;

namespace k8deployer {

void Engine::run()
{
    // Create cluster instances
    for(auto& k: cfg_.kubeconfigs) {
        clusters_.emplace_back(make_unique<Cluster>(cfg_, k, *client_));
    }

    startPortForwardig();

    // Start actions
    for(auto& cluster : clusters_) {
        cluster->run();
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
