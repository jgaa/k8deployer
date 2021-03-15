#pragma once

#include <set>

#include "restc-cpp/restc-cpp.h"
#include "k8deployer/DnsProvisioner.h"

namespace k8deployer {

class DnsProvisionerVubercool : public DnsProvisioner
{
public:
  struct Config {
      std::string user;
      std::string passwd;
      std::string host;
  };

  DnsProvisionerVubercool(Config& cfg);


  void provisionHostname(const std::string &hostname,
                         const std::vector<std::string> &ipv4,
                         const std::vector<std::string> &ipv6) override;

  void deleteHostname(const std::string &hostname) override;

private:
    const Config config_;
    std::shared_ptr<restc_cpp::RestClient> client_;
    std::set<std::string> hostnames_;
};

} // ns
