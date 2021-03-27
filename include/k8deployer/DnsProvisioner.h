#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <boost/asio.hpp>

namespace k8deployer {

class DnsProvisioner
{
public:
   using cb_t = std::function<void(bool success)>;

  DnsProvisioner() = default;
  virtual ~DnsProvisioner() = default;

  virtual void provisionHostname(const std::string& hostname,
                                 const std::vector<std::string>& ipv4,
                                 const std::vector<std::string>& ipv6,
                                 const cb_t& onDone) = 0;

  virtual void deleteHostname(const std::string& hostname, const cb_t& onDone) = 0;

  static std::unique_ptr<DnsProvisioner> create(const std::string& config,
                                                boost::asio::io_service& ioservice);
};

} // ns
