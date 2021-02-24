
#include "k8deployer/logging.h"
//#define RESTC_CPP_LOG_JSON_SERIALIZATION 1
//#define RESTC_CPP_LOG_TRACE LOG_TRACE

#include <sstream>
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/IteratorFromJsonSerializer.h"

#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"
#include "k8deployer/Component.h"
#include "k8deployer/k8/k8api.h"

namespace k8deployer {
struct EventStream {
    std::string type;
    k8api::Event object;
};
} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::StorageDef,
    (k8deployer::k8api::VolumeMount, volume)
    (std::string, capacity)
    (bool, createVolume)
    (std::string, chownUser)
    (std::string, chownGroup)
    (std::string, chmodMode)
    );

BOOST_FUSION_ADAPT_STRUCT(k8deployer::ComponentDataDef,
    // ComponentData
    (std::string, name)
    (k8deployer::labels_t, labels)
    (k8deployer::conf_t, defaultArgs)
    (k8deployer::conf_t, args)
    (k8deployer::k8api::string_list_t, depends)
    (k8deployer::k8api::Job, job)
    (k8deployer::k8api::Deployment, deployment)
    (k8deployer::k8api::StatefulSet, statefulSet)
    (k8deployer::k8api::DaemonSet, daemonset)
    (k8deployer::k8api::Service, service)
    (std::optional<k8deployer::k8api::Secret>, secret)
    (k8deployer::k8api::PersistentVolume, persistentVolume)
    (k8deployer::k8api::Ingress, ingress)   
    (k8deployer::k8api::Namespace, namespace_)
    (k8deployer::k8api::Role, role)
    (k8deployer::k8api::ClusterRole, clusterrole)
    (k8deployer::k8api::RoleBinding, rolebinding)
    (k8deployer::k8api::ClusterRoleBinding, clusterrolebinding)
    (k8deployer::k8api::ServiceAccount, serviceaccount)
    (std::optional<k8deployer::k8api::SecurityContext>, podSecurityContext)
    (std::optional<k8deployer::k8api::PodSecurityContext>, podSpecSecurityContext)
    (std::optional<k8deployer::k8api::Probe>, startupProbe)
    (std::optional<k8deployer::k8api::Probe>, livenessProbe)
    (std::optional<k8deployer::k8api::Probe>, readinessProbe)
    (std::vector<k8deployer::StorageDef>, storage)

    // ComponentDataDef
    (std::string, kind)
    (std::string, parentRelation)
    (k8deployer::ComponentDataDef::childrens_t, children)
    );

BOOST_FUSION_ADAPT_STRUCT(k8deployer::EventStream,
                          (std::string, type)
                          (k8deployer::k8api::Event, object)
                          );

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

namespace {
string ipFromUrl(const string& url) {
    string ip;

    if (const auto pos = url.find("://") ; pos && pos != string::npos) {
        ip = url.substr(pos +3);

        if (auto end = ip.find(":") ; end && end != string::npos) {
            ip = ip.substr(0, end);
        }

        if (auto end = ip.find("/") ; end && end != string::npos) {
            ip = ip.substr(0, end);
        }
    }

    return ip;
}

}

Cluster::Cluster(const Config &cfg, const string &arg)
    : cfg_{cfg}
{
    parseArgs(arg);
    if (!cfg_.storageEngine.empty()) {
        storage_ = Storage::create(cfg_.storageEngine);
    }
}

Cluster::~Cluster()
{

}

//void Cluster::startProxy()
//{
//    portFwd_ = make_unique<PortForward>(client_.GetIoService(), cfg_, kubeconfig_, name());
//    portFwd_->start();
//}

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

std::optional<string> Cluster::getVar(const string &key) const
{
    if (auto it = variables_.find(key); it != variables_.end()) {
        return it->second;
    }

    return {};
}

string Cluster::getUrl() const
{
//    return "http://127.0.0.1:"s
//            + to_string(portFwd_->getPort());
    return url_;
}

std::future<void> Cluster::prepare()
{
    setState(State::INIT);
    LOG_INFO << name () << " Preparing ...";
    loadKubeconfig();
    readDefinitions();
    setCmds();
    createComponents();
    if (!rootComponent_) {
        LOG_WARN << name() << " No components. Nothing to do.";
        return dummyReturnFuture();
    }
    //startEventsLoop();
    assert(prepareCmd_);
    return prepareCmd_();
}

std::future<void> Cluster::execute()
{
    if (executeCmd_) {
        setState(State::EXECUTING);
        LOG_INFO << name () << " " << verb_ << " ...";
        assert(executeCmd_);
        return executeCmd_();
    }

    setState(State::DONE);
    return dummyReturnFuture();
}

void Cluster::loadKubeconfig()
{
    auto kc = Kubeconfig::load(kubeconfig_);

    // Prepare tls for rest client
    auto tls = make_shared<boost::asio::ssl::context>(
                boost::asio::ssl::context{ boost::asio::ssl::context::tls_client });

    tls->set_options(boost::asio::ssl::context::default_workarounds
    | boost::asio::ssl::context::no_sslv2
    | boost::asio::ssl::context::no_sslv3);

    const auto ca = kc->getCaCert();
    tls->add_certificate_authority({ca.data(), ca.size()});

    const auto cs = kc->getClientCert();
    tls->use_certificate({cs.data(), cs.size()}, boost::asio::ssl::context_base::pem);

    const auto key = kc->getClientKey();
    tls->use_private_key({key.data(), key.size()}, boost::asio::ssl::context_base::pem);

    restc_cpp::Request::Properties properties;
    properties.cacheMaxConnectionsPerEndpoint = 32;
    client_ = restc_cpp::RestClient::Create(tls, properties);

    url_ = kc->getServer();

    LOG_INFO << name() << " Will connect directly to: " << url_;
}

void Cluster::startEventsLoop()
{
    LOG_DEBUG << "Starting event-loops";
    client_->Process([this](Context& ctx) {
        const auto url = url_ + "/api/v1/events";

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

void Cluster::readDefinitions()
{
    LOG_DEBUG << name_ << ": Creating components from " << cfg_.definitionFile;

    // Load component definitions
    dataDef_ = make_unique<ComponentDataDef>();

    {
        auto vars = cfg_.variables;

        // Override / merge cluster variables
        for(const auto& [k, v]: variables_) {
            vars[k] = v;
        }

        variables_ = move(vars);
    }

    variables_.insert({"namespace", Engine::config().ns});
    variables_.insert({"clusterIp", ipFromUrl(url_)});

    for(const auto& [k, v]: variables_) {
        LOG_DEBUG << "Cluster " << name_ << " has variable: " << k << '=' << v;
    }

    fileToObject(*dataDef_, cfg_.definitionFile, variables_);
    if (dataDef_->kind.empty()) {
        LOG_ERROR << "Invalid definition file: " << cfg_.definitionFile;
        throw runtime_error("Invalid definition "s + cfg_.definitionFile);
    }
}

void Cluster::createComponents()
{
    rootComponent_ = Component::populateTree(*dataDef_, *this);
}

void Cluster::setCmds()
{
    switch(Engine::mode()) {
    case Engine::Mode::DEPLOY:
        verb_ = "Deploying";
        executeCmd_ = [this] {
            return rootComponent_->deploy();
        };
        prepareCmd_ = [this] {
            rootComponent_->prepare();
            return dummyReturnFuture();
        };
        break;
    case Engine::Mode::DELETE:
        verb_ = "Deleting";
        executeCmd_ = [this] {
            return rootComponent_->remove();
        };
        prepareCmd_ = [this] {
            rootComponent_->prepare();
            return dummyReturnFuture();
        };
        break;
    case Engine::Mode::SHOW_DEPENDENCIES:
            verb_ = "Scanning Dependencies";
            executeCmd_ = [this] {
                return rootComponent_->dumpDependencies();
            };
            prepareCmd_ = [this] {
                rootComponent_->prepare();
                return dummyReturnFuture();
            };
        break;
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
        filesystem::path kubepath{kubeconfig};
        const auto n = kubepath.filename().string();
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
