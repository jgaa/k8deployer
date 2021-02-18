#include "k8deployer/Kubeconfig.h"
#include "k8deployer/Component.h"

#include <boost/fusion/adapted.hpp>

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig::ClusterData,
    (std::string, certificate_authority)
    (std::string, certificate_authority_data)
    (std::string, server)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig::Cluster,
    (k8deployer::Kubeconfig::ClusterData, cluster)
    (std::string, name)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig::UserData,
    (std::string, client_certificate)
    (std::string, client_certificate_data)
    (std::string, client_key)
    (std::string, client_key_data)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig::User,
    (std::string, name)
    (k8deployer::Kubeconfig::UserData, user)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig::ContextData,
    (std::string, cluster)
    (std::string, user)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig::Context,
    (std::string, name)
    (k8deployer::Kubeconfig::ContextData, context)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Kubeconfig,
    (std::string, kind)
    (std::string, apiVersion)
    (std::string, current_context)
    (std::vector<k8deployer::Kubeconfig::Cluster>, clusters)
    (std::vector<k8deployer::Kubeconfig::User>, users)
    (std::vector<k8deployer::Kubeconfig::Context>, contexts)

);

namespace k8deployer {

using namespace std;
using namespace std::string_literals;

namespace {

// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
static std::string base64_decode(const std::string &in) {

    string out;

    vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val=0, valb=-8;
    for (uint8_t c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val>>valb)&0xFF));
            valb -= 8;
        }
    }
    return out;
}

} // ns

string Kubeconfig::getCaCert() const
{
    if (!clusters.at(activeCluster).cluster.certificate_authority_data.empty()) {
        return base64_decode(clusters.at(activeCluster).cluster.certificate_authority_data);
    }

    return slurp(clusters.at(activeCluster).cluster.certificate_authority);
}

string Kubeconfig::getClientCert() const
{
    if (!users.at(activeUser).user.client_certificate_data.empty()) {
        return base64_decode(users.at(activeUser).user.client_certificate_data);
    }

    return slurp(users.at(activeUser).user.client_certificate);
}

string Kubeconfig::getClientKey() const
{
    if (!users.at(activeUser).user.client_key_data.empty()) {
        return base64_decode(users.at(activeUser).user.client_key_data);
    }

    return slurp(users.at(activeUser).user.client_key);
}

string Kubeconfig::getServer() const
{
    return clusters.at(activeCluster).cluster.server;
}

string Kubeconfig::getClusterName() const
{
    return clusters.at(activeCluster).name;
}

unique_ptr<Kubeconfig> Kubeconfig::load(const string &kubefile)
{
    static const restc_cpp::JsonFieldMapping mappings = {
        {"certificate_authority_data", "certificate-authority-data"},
        {"client_certificate_data", "client-certificate-data"},
        {"client_key_data", "client-key-data"},
        {"current_context", "current-context"},
        {"certificate_authority", "certificate-authority"},
        {"client_certificate", "client-certificate"},
        {"client_key", "client-key"}
    };

    const auto path = getKubeconfig(kubefile);

    // Convert yaml file to json
    auto json = fileToJson(path.string(), true);

    // Deserialize json to Kubeconfig instance
    auto kc = make_unique<Kubeconfig>();
    std::istringstream ifs{json};
    restc_cpp::serialize_properties_t properties;
    properties.ignore_unknown_properties = true;
    properties.name_mapping = &mappings;
    restc_cpp::SerializeFromJson(*kc, ifs, properties);

    // Deal with contexts
    bool gotCurrentContext = kc->current_context.empty();
    if (!gotCurrentContext) {
        for(const auto& c : kc->contexts) {
            if (kc->current_context == c.name) {
                // Resolve cluster
                for(const auto& cluster: kc->clusters) {
                    if (cluster.name == c.context.cluster) {
                        // Resolve user
                        for (const auto& user: kc ->users) {
                            if (user.name == c.context.user) {
                                gotCurrentContext = true;
                                break;
                            }
                            ++kc->activeUser;
                        }
                        break;
                    }
                    ++kc->activeCluster;
                }
                break;
            }
        }
    }

    if (!gotCurrentContext) {
        throw runtime_error("Failed to resolve current context: "s + kc->current_context);
    }

    return kc;
}

filesystem::path Kubeconfig::getKubeconfig(const string &kubefile)
{
    filesystem::path path;

    if (!kubefile.empty()) {
        path = kubefile;
    } else if (auto env = getenv("KUBECONFIG")) {
        // For now, support only one file in this environment variable
        path = env;
    } else {
        path = getenv("HOME");
        path /= ".kube";
        path /= "config";
    }

    if (!filesystem::is_regular_file(path)) {
        throw std::runtime_error("No such path: "s + path.string());
    }

    return path;
}


} // ns

