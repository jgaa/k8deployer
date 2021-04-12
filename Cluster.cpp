
#include "k8deployer/logging.h"
//#define RESTC_CPP_LOG_JSON_SERIALIZATION 1
//#define RESTC_CPP_LOG_TRACE LOG_TRACE

#include <sstream>
#include <filesystem>
#include <future>
#include <string_view>
#include <cstdlib>

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

struct PodStream {
    std::string type;
    k8api::Pod object;
};

} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::PodStream,
    (std::string, type)
    (k8deployer::k8api::Pod, object)
    );

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
    (std::string, variant)
    (bool, enabled)
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

Cluster::Cluster(const Config &cfg, const string &arg, const size_t id)
    : cfg_{cfg}
{
    variables_["clusterId"] = to_string(id);
    parseArgs(arg);
    if (!cfg_.storageEngine.empty()) {
        storage_ = Storage::create(cfg_.storageEngine);
    }

    vars_ready_pr_.set_value();
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
    // Prepare the clusters in paralell.
    // They may be inter-connected, and depend on the other clusters
    // being partally initialized.

    setState(State::INIT);
    LOG_INFO << name () << " Preparing ...";
    loadKubeconfig();

    auto pr = make_shared<promise<void>>();

    assert(client_);

    if (!cfg_.dnsServerConfig.empty()) {
        dns_ = DnsProvisioner::create(Engine::config().dnsServerConfig,
                                      client_->GetIoService());
        if (!dns_) {
            LOG_ERROR << "Failed to load dns provisioner from config: " << cfg_.dnsServerConfig;
            throw(runtime_error{"DNS provisioner"});
        }
    }

    client_->GetIoService().post([this, pr] {
       try {
          readDefinitions();
          setCmds();
          createComponents();
          if (rootComponent_) {
              assert(prepareCmd_);
              prepareCmd_();
              prepared_ready_pr_.set_value();
          } else {
              LOG_WARN << name() << " No components. Nothing to do.";
          }

          pr->set_value();
       } catch(const exception& ex) {
          pr->set_exception(current_exception());
       }
    });

    return pr->get_future();
}

std::future<void> Cluster::execute()
{
    if (Engine::mode() == Engine::Mode::DEPLOY && !Engine::config().logDir.empty()) {
        listenForContainers();
    }
    if (executeCmd_) {
        setState(State::EXECUTING);
        LOG_INFO << name () << " " << verb_ << " ...";
        assert(executeCmd_);
        return executeCmd_();
    }

    setState(State::DONE);
    return dummyReturnFuture();
}

std::future<void> Cluster::pendingWork()
{
    client_->GetIoService().post([this] {
        if (openLogs_.empty()) {
            pendingWork_.set_value();
        } else {
            setState(State::LOGGING);
        }
    });

    return pendingWork_.get_future();
}

bool Cluster::addStateListener(const std::string& componentName,
                               const std::function<void (const Component& component)>& fn)
{
    // Called from any thread.
    if (auto c = getComponent(componentName)) {
        c->addStateListener(fn);
        return true;
    }

    return false;
}

void Cluster::add(Component *component)
{
    std::lock_guard<std::mutex> lock{mutex_};
    components_[component->name] = component;
}

Component *Cluster::getComponent(const string &name)
{
    std::lock_guard<std::mutex> lock{mutex_};

    if (auto it = components_.find(name) ; it != components_.end()) {
        return it->second;
    }

    return {};
}

void Cluster::listenForContainers()
{
    assert(client_);
    client_->Process([this](restc_cpp::Context& ctx) {
        const auto url = url_ + "/api/v1/namespaces/"
            + *getVar("namespace")
            + "/pods";

        auto prop = make_shared<Request::Properties>();
        prop->recvTimeout = (60 * 60 * 24) * 1000;

        auto reply = RequestBuilder(ctx)
                .Get(url)
                .Properties(prop)
                .Argument("watch", "true")
                .Argument("labelSelector", "k8dep-deployment="s
                          + rootComponent_->name)
                .Header("X-Client", "k8deployer")
                .Execute();

        serialize_properties_t sp;
        sp.name_mapping = jsonFieldMappings();

        try {
            IteratorFromJsonSerializer<PodStream> pods{*reply, &sp, true};
            for(const auto& pod : pods) {
                LOG_TRACE << name_ << " Container: " << pod.type << " " << pod.object.metadata.name;

                // See if we should start logging for the container
                for(const auto& c: pod.object.status.containerStatuses) {
                    auto& prevState = knownContainers_[c.containerID];
                    if (prevState.name.empty()) {
                        // New container. Delete any existing log file?
                        prepareLogging(pod.object, c);
                    }

                    if (c.started != prevState.started) {
                        if (c.started) {
                            startLogging(pod.object, c);
                        } else {
                            stopLogging(pod.object, c);
                        }
                    }

                    prevState = c;
                }

                if (state() > State::EXECUTING) {
                    LOG_DEBUG << name_ << " State is > EXECUTING. Exciting container watch loop.";
                    break;
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
    properties.cacheMaxConnectionsPerEndpoint = 64;
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

    variables_.insert({"namespace",
                       Engine::config().useClusterNameAsNamespace ? name() : Engine::config().ns});
    variables_.insert({"clusterIp", ipFromUrl(url_)});

    for(const auto& [k, v]: variables_) {
        LOG_DEBUG << "Cluster " << name_ << " has variable: " << k << '=' << v;
    }

    fileToObject(*dataDef_, cfg_.definitionFile, variables_, true);
    if (dataDef_->kind.empty()) {
        LOG_ERROR << "Invalid definition file: " << cfg_.definitionFile;
        throw runtime_error("Invalid definition "s + cfg_.definitionFile);
    }

    definitions_ready_pr_.set_value();
}

void Cluster::createComponents()
{
    rootComponent_ = Component::populateTree(*dataDef_, *this);
    basic_components_pr_.set_value();
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
        auto n = kubepath.filename().string();

        if (Engine::config().useFirstPartOfKubeConfigAsClusterName) {
            if (auto pos = n.find('.') ; pos && pos != string::npos) {
                n = n.substr(0, pos);
            }
        }

        if (n.empty()) {
            variables_["name"] = "default";
        } else {
            variables_["name"] = n;
        }
    }

    name_ = variables_["name"];

    LOG_TRACE << "Cluster " << name() << " has variables: " << getVars();
}

void Cluster::prepareLogging(const k8api::Pod &pod, const k8api::ContainerStatus &container)
{
    auto path = logPath(pod, container);
    if (path.empty()) {
        return;
    }

    if (!filesystem::is_directory(path.parent_path())) {
        LOG_INFO << name() << " Creating log directory: " << path.parent_path().string();
        filesystem::create_directories(path.parent_path());
    } else if (filesystem::is_regular_file(path)) {
        LOG_DEBUG << name() << " Truncating log-file: " << path.string();
        filesystem::resize_file(path, 0);
    }
}

void Cluster::startLogging(const k8api::Pod &pod, const k8api::ContainerStatus &container)
{
    assert(client_);
    client_->Process([this, pod, container](restc_cpp::Context& ctx) {
        // TODO: How do we signal to stop logging?

        const auto path = logPath(pod, container);
        ofstream out{path, ::ios_base::app};
        if (!out.is_open()) {
            LOG_WARN << "Failed to open log-file: " << path.string();
        }

        LOG_INFO << name() << " Opening log: " << path.string();

        if (!Engine::config().logViewer.empty()) {
            const auto cmd = Engine::config().logViewer + " " + path.string() + " &";
            LOG_TRACE << name() << " Executing: " << cmd;
            system(cmd.c_str());
        }

        const auto url = url_ + "/api/v1/namespaces/"
            + *getVar("namespace")
            + "/pods/" + pod.metadata.name + "/log";

        auto prop = make_shared<Request::Properties>();
        prop->recvTimeout = (60 * 60 * 24) * 1000;

        openLogs_[container.containerID] = path.string();

        auto reply = RequestBuilder(ctx)
                .Get(url)
                .Properties(prop)
                .Argument("follow", "true")
                .Header("X-Client", "k8deployer")
                .Execute();

        while (true) {
            const auto& b = reply->GetSomeData();

            if (b.size() == 0) {
                LOG_TRACE << name() << " End of log-file: " << path.string();
                break;
            }

            const string_view s{static_cast<const char *>(b.data()), b.size()};
            out << s;
        }

        LOG_INFO << name() << " Closing log: " << path.string();

        openLogs_.erase(container.containerID);
        if (openLogs_.empty()) {
            pendingWork_.set_value();
            if (state() == State::LOGGING) {
                setState(State::DONE);
            }
        }

      });
}

void Cluster::stopLogging(const k8api::Pod &pod, const k8api::ContainerStatus &container)
{
    LOG_DEBUG << name() << " Ignoring stop log on: " << logPath(pod, container);
}

filesystem::path Cluster::logPath(const k8api::Pod &pod, const k8api::ContainerStatus &container)
{
    if (Engine::config().logDir.empty()) {
        return {};
    }

    filesystem::path p = Engine::config().logDir;
    p /= name();
    p /= pod.metadata.namespace_;
    p /= pod.metadata.name;

    if (pod.spec.containers.size() > 1) {
        p += "_" + container.name;
    }

    p += ".log";

    return p;
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
