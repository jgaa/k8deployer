#pragma once

#include "k8deployer/DnsProvisioner.h"
#include "k8deployer/Config.h"
#include "restc-cpp/restc-cpp.h"

namespace k8deployer {

/*! Simple interface to make k8deployer act as a certbot DNS hook
 *
 * This mode is intended to be triggered as:
 *
 *    k8deployer in deploy mode needing to create new cert(s)
 *      |
 *      v
 *     certbot certonly --manual --preferred-challenges=dns --manual-auth-hook ...
 *        |
 *        v
 *       k8deployer in dns mode, configuring a TXT entry for certbot validation
 *
 *  Envvars:
 *
 *      - DNS_CONFIG: The value of --dns-server-config in the calling k8deployer
 *      -
 */
class DnsMode
{
public:
    DnsMode(const Config& config)
        : config_{config} {
        init();
    }

    void run();

private:
    void authenticator();
    void cleanup();
    void init();
    std::string getEnv(std::string_view name);
    std::string getHostname() const;

    const Config& config_;
    std::shared_ptr<restc_cpp::RestClient> client_;
    std::unique_ptr<DnsProvisioner> dns_;
};

} // ns
