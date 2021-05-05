
#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>
#include "k8deployer/logging.h"
#include "k8deployer/Config.h"
#include "k8deployer/Engine.h"
#include "k8deployer/Component.h"

using namespace std;
using namespace k8deployer;

int main(int argc, char* argv[]) {

    try {
        locale loc("");
    } catch (const std::exception& e) {
        std::cout << "Locales in Linux are fundamentally broken. Never worked. Never will. Overriding the current mess with LC_ALL=C" << endl;
        setenv("LC_ALL", "C", 1);
    }

    Config config;
    {
        std::string log_level = "info";

        namespace po = boost::program_options;
        po::options_description general("Options");
        po::positional_options_description positionalDescription;

        general.add_options()
            ("help,h", "Print help and exit")
            ("version", "print version string and exit")
            ("definition-file,d",
                po::value<string>(&config.definitionFile)->default_value(config.definitionFile),
                "Definition file to deploy")
            ("namespace,n",
                po::value<string>(&config.ns)->default_value(config.ns),
                "Deploy in this namespace")
            ("exclude,e",
                po::value<string>(&config.excludeFilter)->default_value(config.excludeFilter),
                "Exclude filter for components. This is a regex against the component's names")
            ("enable",
                po::value<string>(&config.enabledFilter)->default_value(config.enabledFilter),
                "Enable disabled componnet(s). This is a regex against the component's names. "
                "This can be used to enable commonly disabled component(s) without altering the definition file.")
            ("include,i",
                 po::value<string>(&config.includeFilter)->default_value(config.includeFilter),
                 "Include filter for components. This is a regex against the component's names. "
                 "If used, all non-matching components are excluded.")
            ("manage-namespace,N",
                 po::value<bool>(&config.autoMaintainNamespace)->default_value(config.autoMaintainNamespace),
                 "Create and delete namespaces as needed")
            ("log-level,l",
                 po::value<string>(&log_level)->default_value(log_level),
                 "Log-level to use; one of 'info', 'debug', 'trace'")
            ("command,c",
                 po::value<string>(&config.command)->default_value(config.command),
                 "Comand; one of: 'deploy', 'delete', 'depends'")
            ("storage,s",
                 po::value<string>(&config.storageEngine)->default_value(config.storageEngine),
                 "Storage engine for managed volumes")
            ("pvc-storage-class-name",
                 po::value<string>(&config.pvcStorageClassName)->default_value(config.pvcStorageClassName),
                 "storageClassName to use in Persistent Storage Volume Claims."
                 "For example `local-storage` for sig 'Local Persistence Volume Static Provisioner' pre-provisioned volumes.")
            ("randomize-paths,r",
                 po::value<bool>(&config.randomizePaths)->default_value(config.randomizePaths),
                 "Add a uuid to provisioned paths (nfs) to always depoloy on a 'fresh' volume.")
            ("use-networking-betav1",
                 po::value<bool>(&config.useNetworkingBetaV1)->default_value(config.useNetworkingBetaV1),
                 "Use networking betav1 (pre kubernetes 1.19) - backwards compatibility")
            ("use-cluster-name-as-namespace,C",
                 po::value<bool>(&config.useClusterNameAsNamespace)->default_value(config.useClusterNameAsNamespace),
                 "Use the cluster-name as k8s namespace for the individual clusters.")
            ("skip-dep-init-containers",
                 po::value<bool>(&config.skipDependencyInitContainers)->default_value(config.skipDependencyInitContainers),
                 "Skip creation of init-containers used to enforce startup-order after the deploment")
            ("dns-server-config,D",
                 po::value<string>(&config.dnsServerConfig)->default_value(config.dnsServerConfig),
                 "Configuration file for automatic DNS provisioning of ingress host names."
                 "If unset, this feature is disabled.")
            ("use-load-balancer-ip,L",
                 po::value<bool>(&config.useLoadBalancerIp)->default_value(config.useLoadBalancerIp),
                 "Use the load-balancers reported IP for ingress DNS provisioning."
                 "If `kubectl -n ... get ingress` shows the IP, this should work.")
            ("use-first-part-of-kubeconf-as-cluser-name,F",
                 po::value<bool>(&config.useFirstPartOfKubeConfigAsClusterName)->default_value(config.useFirstPartOfKubeConfigAsClusterName),
                 "If the cube-config file specified by `-k` arg contains dots, "
                 "use the part from the start of the file-name to the first dot as cluster name")
            ("variables,v",
                 po::value<decltype(config.rawVariables)>(&config.rawVariables),
                 "One or more variables var=value WS var=value...")
            ("remove-env-var,R",
                 po::value<decltype(config.removeEnvVars)>(&config.removeEnvVars),
                 "Remove environment-variable declaration with this name"
                 "Used to filter out envvars for certain deployments.")
            ("ignore-resource-limits",
                 po::value<bool>(&config.ignoreResourceLimits)->default_value(config.ignoreResourceLimits),
                 "Do not set resource limits in the container, even if they are declared in the definitions.")
            ("variant,V",
                 po::value<decltype(config.variants)>(&config.variants),
                 "Variant override: componentNameRegEx=variant. This argument can be repeated. "
                 "This can be used declare several components with the same name but different variant names "
                 "and then specify which one to use at the command line. Useful for deploying telepresence or debugging."
                 "By default, a component without a variant name will be used."
                 "pods.")
            ("log-dir",
                 po::value<string>(&config.logDir)->default_value(config.logDir),
                 "If specified, log container logs to that directory as the containers are started")
            ("wipe-log-dir",
                 po::value<bool>(&config.wipeLogDir)->default_value(config.wipeLogDir),
                 "Delete all files and subdirectories in the log-dir before logging starts.")
            ("log-viewer",
                 po::value<string>(&config.logViewer)->default_value(config.logViewer),
                 "If specified, call this command with the log-file patch each time a log-file is opened.")
            ("web-browser,b",
                 po::value<string>(&config.webBrowser)->default_value(config.webBrowser),
                 "If specified, calls this command with the value of 'openInBrowser' "
                 "when components with this argument are ready");

        po::options_description hidden("Clusters");
        hidden.add_options()("kubeconfig,k",
                             po::value<decltype(config.kubeconfigs)>(&config.kubeconfigs),
                             "One or more kubeconfigs to clusters to deploy the definition. "
                             "Optional syntax: kubefile:var=value,var=value...");

        po::options_description cmdline_options;
        po::positional_options_description kfo;
        cmdline_options.add(general).add(hidden);
        kfo.add("kubeconfig", -1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(kfo).run(), vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << filesystem::path(argv[0]).stem().string() << " [options]";
            std::cout << cmdline_options << std::endl;
            return -1;
        }

        if (vm.count("version")) {
            std::cout << filesystem::path(argv[0]).stem().string() << ' '  << K8DEPLOYER_VERSION << endl;
            return -2;
        }

        // Expand variable-definitions into the config's variables property
        for(const auto& vars : config.rawVariables) {
            auto kv = Component::getArgAsKv(vars);
            for(const auto& [k, v] : kv) {
                config.variables[k] = v;
            }
        }

        auto llevel = logfault::LogLevel::INFO;
        if (log_level == "debug") {
            llevel = logfault::LogLevel::DEBUGGING;
        } else if (log_level == "trace") {
            llevel = logfault::LogLevel::TRACE;
        } else if (log_level == "info") {
            ;  // Do nothing
        } else {
            std::cerr << "Unknown log-level: " << log_level << endl;
            return -1;
        }

        logfault::LogManager::Instance().AddHandler(
                    make_unique<logfault::StreamHandler>(clog, llevel));

        if (config.kubeconfigs.empty()) {
            config.kubeconfigs.push_back(""); // Use the default kubeconfig, whatever that is
        }

        LOG_INFO << filesystem::path(argv[0]).stem().string() << ' ' << K8DEPLOYER_VERSION  ". Log level: " << log_level;
    }

    try {
        Engine engine{config};
        engine.run();
    } catch (const future_error& ) {
        ; // TODO: Fix it, dont just hide it
    } catch (const exception& ex) {
        LOG_ERROR << "Caught exception from run: " << ex.what();
    }
}
