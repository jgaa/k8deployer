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
    filesystem::path cbdir;
    if (const auto home = getenv("HOME")) {
        cbdir = home;
    } else {
        LOG_WARN << "No HOME environemnt variable is set. Using current dir as home-dir";
        cbdir = filesystem::current_path();
    }

    cbdir /= ".k8deployer";
    cbdir /= Engine::instance().getCluster(0)->getRootComponent().name;
    cbdir /= "certbot";

    if (!filesystem::is_directory(cbdir)) {
        LOG_DEBUG << "Creating directory for certbot integration: " << cbdir;
        filesystem::create_directories(cbdir);
    }

    certbot_path_ = cbdir;
}

void Certbot::runCertbot()
{

}

void Certbot::createSecrets()
{

}

void Certbot::addSecretsToIngress()
{

}



} // ns
