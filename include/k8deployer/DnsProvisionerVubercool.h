#pragma once

#include <set>

#include "restc-cpp/restc-cpp.h"
#include "k8deployer/DnsProvisioner.h"

namespace k8deployer {

struct VubercoolReq {
  std::vector<std::string> a;
  std::vector<std::string> aaaa;
  std::string txt;
};

class DnsProvisionerVubercool : public DnsProvisioner
{
public: 
  struct Config {
      std::string user;
      std::string passwd;
      std::string host;
      std::string url;
      size_t retries = 0;
      size_t retryDelaySecond = 1;
  };

  DnsProvisionerVubercool(const Config& cfg, boost::asio::io_service& ioservice);


  // A / AA entry
  void provisionHostname(const std::string &hostname,
                         const std::vector<std::string> &ipv4,
                         const std::vector<std::string> &ipv6,
                         const cb_t& onDone, bool onlyOnce = true) override;

  void provisionHostname(const std::string &hostname,
                           const std::string txt,
                           const cb_t& onDone, bool onlyOnce = true) override;

  void deleteHostname(const std::string &hostname, const cb_t& onDone) override;

private:
    void patchDnsServer(const std::string &hostname, const VubercoolReq& req,
                        const cb_t& onDone, bool onlyOnce = true);
    bool addHostname(const std::string& hostname);
    void deleteHostname(const std::string& hostname);
    std::string getUrl() const;

    const Config config_;
    std::shared_ptr<restc_cpp::RestClient> client_;
};

} // ns
