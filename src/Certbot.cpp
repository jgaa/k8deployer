#include <filesystem>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include "k8deployer/Certbot.h"
#include "k8deployer/Engine.h"

namespace k8deployer {

using namespace std;

Certbot &Certbot::instance()
{
    static Certbot instance_;
    return instance_;
}

void Certbot::add(BaseComponent &component)
{
    LOG_TRACE << "certbot: Adding component " << component.logName();
    lock_guard<mutex> lock{mutex_};
    components_.emplace_back(&component);
}

void Certbot::createOrUpdateCerts()
{
    scanForHostnames();
    prepareStorageDir();
    runCertbot();
    createSecrets();
    addSecretsToIngress();
}

void Certbot::scanForHostnames()
{
    // TODO: refactor as a general "walk children, look for kind" algorithm with a lambda
    for(auto c : components_) {
        assert(c);
        auto gakke = c->ingress;
        for(auto& child : c->getChildren()) {
            if (child->getKind() == Kind::SERVICE) {
                for(auto& schild : child->getChildren()) {
                    if (schild->getKind() == Kind::INGRESS) {
                        if (auto spec = schild->ingress.spec) {
                            size_t hostname_cnt = 0;
                            for(auto& rule : spec->rules) {
                                if (rule.host.empty()) {
                                    LOG_WARN << c->logName()
                                             << "Found empty hostname in ingress spec!";
                                    continue;
                                }
                                hostnames.emplace(rule.host);
                                ++hostname_cnt;
                                LOG_TRACE  << c->logName()
                                           << "Certbot::scanForHostnames: Found hostname '" << rule.host << '\'';
                            }

                            if (hostname_cnt) {
                                // Add a pointer to the secret will will create using a letsencrypt TLS certificate
                                k8api::IngressTLS tls;
                                tls.secretName = "k8deployerCertbotGenerated";
                                spec->tls.emplace_back(move(tls));
                                LOG_TRACE << c->logName()
                                          << "Added certbot secret to ingress";
                            }
                        }
                    }
                }
            }
        }
    }
}

void Certbot::prepareStorageDir()
{
    auto cbdir = Engine::instance().getProjectPath();
    cbdir /= "certbot";

    makeIfNotExists(cbdir);
    certbot_path_ = cbdir;
}

void Certbot::runCertbot()
{
    // Create scripts
    auto bindir = certbot_path_;
    bindir /= "bin";
    makeIfNotExists(bindir);

    const auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());

    auto auth_hook = bindir;
    auth_hook /= ("auth_"s
                  + "hook"
                  //+ uuid
                  + ".sh");

    const auto& conf = Engine::config();

    {
        ofstream out{auth_hook, ios_base::out | ios_base::trunc};
        filesystem::permissions(auth_hook, filesystem::perms::owner_read | filesystem::perms::owner_write | filesystem::perms::owner_exec);

        out << "#!/bin/bash" << endl
            << conf.appBinary
            << " -c certbot-authenticator -l trace -d '" << conf.definitionFile
            << "' -D '" << conf.dnsServerConfig
            << "'"  << endl;
    }

    // TODO: Set up a scope to delete the scripts

    // Setup environment variables

    // Call certbot

    std::string domains;
    for(const auto& host : hostnames) {
        assert(!host.empty());
        if (!domains.empty()) {
            domains += ",";
        }
        domains += host;
    }

    auto cmd = "certbot -n certonly --test-cert --manual "
        "--preferred-challenges=dns "
        "--manual-auth-hook '"s + auth_hook.string()
        + "' --work-dir '" + certbot_path_.string()
        + "' --logs-dir '" + certbot_path_.string()
        + "' --config-dir '" + certbot_path_.string()
        + "' --register-unsafely-without-email --agree-tos --manual-public-ip-logging-ok "
        + " -d " + domains;

    LOG_DEBUG << "Executing shell command: " << cmd;
    auto result = system(cmd.data());
    if (result) {
        LOG_ERROR << "Failed to create certs";
    }

    // TODO: Use the generated certs
}

void Certbot::createSecrets()
{

}

void Certbot::addSecretsToIngress()
{

}

void Certbot::makeIfNotExists(const filesystem::path &path)
{
    if (!filesystem::is_directory(path)) {
        LOG_DEBUG << "Creating directory: " << path;
        filesystem::create_directories(path);
    }
}



} // ns
