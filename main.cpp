
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

        general.add_options()("help,h", "Print help and exit")(
                    "version", "print version string and exit")(
                    "definition-file,d",
                    po::value<string>(&config.definitionFile)->default_value(config.definitionFile),
                    "Definition file to deploy")(
                    "namespace,n",
                    po::value<string>(&config.ns)->default_value(config.ns),
                    "Deploy in this namespace")(
                    "exclude,e",
                    po::value<string>(&config.excludeFilter)->default_value(config.excludeFilter),
                    "Exclude filter for components. This is a regex against the component's names")(
                    "enable",
                    po::value<string>(&config.enabledFilter)->default_value(config.enabledFilter),
                    "Enable disabled componnet(s). This is a regex against the component's names. "
                    "This can be used to enable commonly disabled component(s) without altering the definition file.")(
                    "include,i",
                    po::value<string>(&config.includeFilter)->default_value(config.includeFilter),
                    "Include filter for components. This is a regex against the component's names. "
                    "If used, all non-matching components are excluded.")(
                    "manage-namespace,N",
                    po::value<bool>(&config.autoMaintainNamespace)->default_value(config.autoMaintainNamespace),
                    "Create and delete namespaces as needed")(
                    "log-level,l",
                    po::value<string>(&log_level)->default_value(log_level),
                    "Log-level to use; one of 'info', 'debug', 'trace'")(
                    "command,c",
                    po::value<string>(&config.command)->default_value(config.command),
                    "Comand; one of: 'deploy', 'delete', 'depends'")(
                    "storage,s",
                    po::value<string>(&config.storageEngine)->default_value(config.storageEngine),
                    "Storage engine for managed volumes")(
                    "randomize-paths,r",
                    po::value<bool>(&config.randomizePaths)->default_value(config.randomizePaths),
                    "Add a uuid to provisioned paths (nfs) to always depoloy on a 'fresh' volume.")(
                    "use-networking-betav1",
                    po::value<bool>(&config.useNetworkingBetaV1)->default_value(config.useNetworkingBetaV1),
                    "Use networking betav1 (pre kubernetes 1.19) - backwards compatibility")(
                    "skip-dep-init-containers",
                    po::value<bool>(&config.skipDependencyInitContainers)->default_value(config.skipDependencyInitContainers),
                    "Skip creation of init-containers used to enforce startup-order after the deploment")(
                    "variables,v",
                    po::value<decltype(config.rawVariables)>(&config.rawVariables),
                    "One or more variables var=value WS var=value...")(
                    "variant,V",
                    po::value<decltype(config.variants)>(&config.variants),
                    "Variant override: componentNameRegEx=variant. This argument can be repeated. "
                    "This can be used declare several components with the same name but different variant names "
                    "and then specify which one to use at the command line. Useful for deploying telepresence or debugging."
                    "By default, a component without a variant name will be used."
                    "pods.")(
                    "kubeconfig,k",
                    po::value<decltype(config.kubeconfigs)>(&config.kubeconfigs),
                    "One or more kubeconfigs to clusters to deploy the definition. "
                    "Optional syntax: kubefile:var=value,var=value...");

        po::options_description cmdline_options;
        cmdline_options.add(general);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
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
    } catch (const exception& ex) {
        LOG_ERROR << "Caught exception from run: " << ex.what();
    }
}
