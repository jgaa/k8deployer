#pragma once

#include <vector>
#include <string>
#include <memory>

namespace k8deployer {

class DnsProvisioner
{
public:
  DnsProvisioner() = default;
  virtual ~DnsProvisioner() = default;

  virtual void provisionHostname(const std::string& hostname,
                                 const std::vector<std::string>& ipv4,
                                 const std::vector<std::string>& ipv6) = 0;

  virtual void deleteHostname(const std::string& hostname) = 0;

  static std::unique_ptr<DnsProvisioner> create(const std::string& config);
};

} // ns
