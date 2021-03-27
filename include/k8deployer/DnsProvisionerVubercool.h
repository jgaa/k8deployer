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
      size_t retries = 0;
      size_t retryDelaySecond = 1;
  };

  DnsProvisionerVubercool(const Config& cfg, boost::asio::io_service& ioservice);


  void provisionHostname(const std::string &hostname,
                         const std::vector<std::string> &ipv4,
                         const std::vector<std::string> &ipv6,
                         const cb_t& onDone) override;

  void deleteHostname(const std::string &hostname, const cb_t& onDone) override;

private:
    bool addHostname(const std::string& hostname);
    void deleteHostname(const std::string& hostname);

    const Config config_;
    std::shared_ptr<restc_cpp::RestClient> client_;
};

} // ns
