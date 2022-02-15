#pragma once

#include <filesystem>

#include "k8deployer/Config.h"
#include "k8deployer/BaseComponent.h"

namespace k8deployer {

class Certbot {
public:
    static Certbot& instance();

    /*! Add a component to the list of components to scan for hostnames
     */
    void add(BaseComponent& component);
    void createOrUpdateCerts();

private:
    void scanForHostnames();
    void prepareStorageDir();
    void runCertbot();
    void createSecrets();
    void addSecretsToIngress();

    std::mutex mutex_;
    std::deque<BaseComponent *> components_;
    std::set<std::string> hostnames;
    std::filesystem::path certbot_path_;
};

}
