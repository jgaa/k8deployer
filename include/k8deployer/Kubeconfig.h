#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace k8deployer {

class Kubeconfig
{
public:
    Kubeconfig() = default;

    std::string getCaCert() const;
    std::string getClientCert() const;
    std::string getClientKey() const;
    std::string getServer() const;
    std::string getClusterName() const;

    /*! Tries to load the configuration the same way as `kubectl`.
     *     - If the kubefile is set, use that
     *     - If KUBECONFIG environemnt vbariable is set, use that
     *     - Try ~/.kube/config
     *
     *     @return pointer if config was found.
     *     @throws std::runtime_error if the config was not available or invalid
     */
    static std::unique_ptr<Kubeconfig> load(const std::string& kubefile);

    struct ClusterData
    {
        std::string certificate_authority; // file path
        std::string certificate_authority_data;
        std::string server; // url
    };

    struct Cluster {
        ClusterData cluster;
        std::string name;
    };

    struct UserData {
        std::string client_certificate; // file path
        std::string client_certificate_data;
        std::string client_key; // file path
        std::string client_key_data;
    };

    struct User {
        std::string name;
        UserData user;
    };

    struct ContextData {
        std::string cluster;
        std::string user;
    };

    struct Context {
        std::string name;
        ContextData context;
    };

    std::string kind;
    std::string apiVersion;
    std::string current_context;
    std::vector<Cluster> clusters;
    std::vector<User> users;
    std::vector<Context> contexts;

    size_t activeCluster = 0;
    size_t activeUser = 0;

private:
    static std::filesystem::path getKubeconfig(const std::string& kubefile);
};

} // ns
