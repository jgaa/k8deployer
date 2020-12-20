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

BOOST_FUSION_ADAPT_STRUCT(k8deployer::ComponentDataDef,
                          (std::string, name)
                          (k8deployer::labels_t, labels)
                          (k8deployer::conf_t, defaultArgs)
                          (k8deployer::conf_t, args)
                          (k8deployer::k8api::Deployment, deployment)
                          (k8deployer::k8api::Service, service)
                          (std::string, kind)
                          (std::string, parentRelation)
                          (k8deployer::ComponentDataDef::childrens_t, children)
                          );


namespace k8deployer {

Engine *Engine::instance_;

void Engine::run()
{
    readDefinitions();

    // Create cluster instances
    for(auto& k: cfg_.kubeconfigs) {
        clusters_.emplace_back(make_unique<Cluster>(cfg_, k, *client_, dataDef_));
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

    // Let the events backlog be safely ignored...
    // TODO: Use the futures above to hold the clusters back until the event backlog is received
    this_thread::sleep_for(5s);
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

void Engine::readDefinitions()
{
    LOG_DEBUG << "Creating components from " << cfg_.definitionFile;

    if (!filesystem::is_regular_file(cfg_.definitionFile)) {
        LOG_ERROR << "Not a file: " << cfg_.definitionFile;
        throw runtime_error("Not a file: "s + cfg_.definitionFile);
    }

    string json;
    const filesystem::path path{cfg_.definitionFile};
    const auto ext = path.extension();
    if (ext == ".yaml") {
        // https://www.commandlinefu.com/commands/view/12218/convert-yaml-to-json
        //python -c 'import sys, yaml, json; json.dump(yaml.load(open("nginx.yaml","r").read()), sys.stdout, indent=4)'
        const auto expr = R"(import sys, yaml, json; json.dump(yaml.load(open(")"s + cfg_.definitionFile + R"(","r").read()), sys.stdout, indent=4))"s;
        auto args = boost::process::args({"-c"s, expr});
        boost::process::ipstream pipe_stream;
        boost::process::child process(boost::process::search_path("python"),
                                      args,
                                      boost::process::std_out > pipe_stream);
        string line;
        while (pipe_stream && std::getline(pipe_stream, line)) {
            json += line;
        }

        error_code ec;
        process.wait(ec);
        if (ec) {
            LOG_ERROR << "Failed to convert yaml from " << cfg_.definitionFile << ": " << ec.message();
            throw runtime_error("Failed to convert yaml: "s + cfg_.definitionFile);
        }

    } else if (ext == ".json") {
        json = slurp(cfg_.definitionFile);
    } else {
        LOG_ERROR << "File extension must be yaml or json: " << cfg_.definitionFile;
        throw runtime_error("Unknown extension "s + cfg_.definitionFile);
    }

    // Load component definitions
    istringstream ifs{json};
    restc_cpp::SerializeFromJson(dataDef_, ifs);
    if (dataDef_.kind.empty()) {
        LOG_ERROR << "Invalid definition file: " << cfg_.definitionFile;
        throw runtime_error("Invalid definition "s + cfg_.definitionFile);
    }
}

}
