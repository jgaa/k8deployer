
#include "k8deployer/logging.h"
//#define RESTC_CPP_LOG_JSON_SERIALIZATION 1
//#define RESTC_CPP_LOG_TRACE LOG_TRACE

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/IteratorFromJsonSerializer.h"

#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/k8/k8api.h"

namespace k8deployer {
struct EventStream {
    std::string type;
    k8api::Event object;
};
} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::EventStream,
                          (std::string, type)
                          (k8deployer::k8api::Event, object)
                          );

namespace k8deployer {

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

Cluster::Cluster(const Config &cfg, const string &arg, RestClient &client)
    : client_{client}, cfg_{cfg}
{
    parseArgs(arg);
}

void Cluster::startProxy()
{
    portFwd_ = make_unique<PortForward>(client_.GetIoService(), cfg_, kubeconfig_, name());
    portFwd_->start();
}

string Cluster::getVars() const
{
    ostringstream out;

    size_t cnt = 0;
    for(const auto& [n, v] : variables_) {
        if (cnt++) {
            out << ", ";
        }
        out << n << '=' << v;
    }
    return out.str();
}

string Cluster::getUrl() const
{
    return "http://127.0.0.1:"s
            + to_string(portFwd_->getPort());
}

std::future<void> Cluster::prepare()
{
    setState(State::INIT);
    LOG_INFO << name () << " Preparing ...";
    setCmds();
    startEventsLoop();
    createComponents();
    assert(prepareCmd_);
    return prepareCmd_();
}

std::future<void> Cluster::execute()
{
    setState(State::EXECUTING);
    LOG_INFO << name () << " Executing ...";
    assert(executeCmd_);
    return executeCmd_();
}

void Cluster::startEventsLoop()
{
    LOG_DEBUG << "Starting event-loops";
    client_.Process([this](Context& ctx) {
        const auto url = "http://127.0.0.1:"s + to_string(portFwd_->getPort())
                + "/api/v1/events";

        auto prop = make_shared<Request::Properties>();
        prop->recvTimeout = (60 * 60 * 24) * 1000;

        auto reply = RequestBuilder(ctx)
                .Get(url)
                .Properties(prop)
                .Header("X-Client", "k8deployer")
                .Argument("watch","true")
                .Execute();

        // 'namespace' is a reserved word in C++, so we have to map it
        serialize_properties_t sp;
        sp.name_mapping = jsonFieldMappings();

        IteratorFromJsonSerializer<EventStream> events{*reply, &sp, true};

        try {
            for(const auto& item : events) {
                // This gets called asynchrounesly for each event we get from the server
                const auto& event = item.object;
                LOG_TRACE << name() << ": got event: "
                          << event.metadata.namespace_ << '.'
                          << event.metadata.name
                          << " [" << event.reason
                          << "] " << event.message;

                if (rootComponent_) {
                    auto ep = make_shared<k8api::Event>(event);
                    rootComponent_->onEvent(ep);
                }
            }
        } catch (const exception& ex) {
            LOG_ERROR << "Caught exception from event-loop: " << ex.what();

            // If we get an exception before we are shutting down, it's an error
            // TODO: Try to recover if we are in PROCESSING state
            if (state() < State::ERROR) {
                setState(State::ERROR);
            }
        }
    });
}

void Cluster::createComponents()
{
    LOG_DEBUG << name()<< ": Creating components from " << cfg_.definitionFile;

    assert(!rootComponent_);
    if (!boost::filesystem::is_regular_file(cfg_.definitionFile)) {
        LOG_ERROR << name() << ": Not a file: " << cfg_.definitionFile;
        throw runtime_error("Not a file: "s + cfg_.definitionFile);
    }


    // Load component definitions
    ifstream ifs{cfg_.definitionFile};
    ComponentDataDef def;
    restc_cpp::SerializeFromJson(def, ifs);

    // Create the tree of components from the definition
    rootComponent_ = Component::populateTree(def, *this);
}

void Cluster::setCmds()
{
    if (cfg_.command == "deploy") {
        executeCmd_ = [this] {
            return rootComponent_->deploy();
        };
        prepareCmd_ = [this] {
            return rootComponent_->prepareDeploy();
        };
    } else  {
        LOG_ERROR << "Unknown command: " << cfg_.command;
    }
}


void Cluster::parseArgs(const std::string& args)
{
    auto [kubeconfig, vars] = split(args, ':');
    kubeconfig_ = kubeconfig;

    vector<string> pairs;
    boost::split(pairs, vars, boost::is_any_of(","));

    for(const auto& pair : pairs) {
        if (!pair.empty()) {
            variables_.insert(split(pair, '='));
        }
    }

    if (variables_["name"].empty()) {
        auto [n, _] = split(kubeconfig_, '.');
        if (n.empty()) {
            variables_["name"] = "default";
        } else {
            variables_["name"] = n;
        }
    }

    name_ = variables_["name"];

    LOG_TRACE << "Cluster " << name() << " has variables: " << getVars();
}

std::pair<string, string> Cluster::split(const string &str, char ch) const
{
    auto pos = str.find(ch);
    if (pos == string::npos) {
        return {str, {}};
    }

    return {str.substr(0, pos), str.substr(pos +1)};
}

} // ns
