
#include <boost/fusion/adapted.hpp>
#include "k8deployer/DnsProvisionerVubercool.h"
#include "k8deployer/logging.h"

#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/RequestBuilder.h"

namespace k8deployer {

struct VubercoolReq {
  std::vector<std::string> a;
  std::vector<std::string> aaaa;
};

} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::VubercoolReq,
    (std::vector<std::string>, a)
    (std::vector<std::string>, aaaa)
);

namespace k8deployer {

using namespace std;
using namespace std::string_literals;
using namespace restc_cpp;

DnsProvisionerVubercool::DnsProvisionerVubercool(DnsProvisionerVubercool::Config &cfg)
  : config_{cfg}
{
    client_ = RestClient::Create();
}

void DnsProvisionerVubercool::provisionHostname(const std::string &hostname,
                                                const std::vector<std::string> &ipv4,
                                                const std::vector<std::string> &ipv6)
{
  this->client_->ProcessWithPromise([&](Context& ctx) {
    auto url = "https://"s + config_.host + "/zone/" + hostname;

    LOG_DEBUG << "Provisioning hostname " << hostname;

    if (hostnames_.find(hostname) != hostnames_.end()) {
        LOG_DEBUG << "Hostname " << hostname << " was already provisioned. Skipping...";
        return;
    }

    VubercoolReq body;
    body.a = ipv4;
    body.aaaa = ipv6;

    try {
      auto reply = RequestBuilder(ctx)
        .Patch(url)
        .BasicAuthentication(config_.user, config_.passwd)
        .Header("X-Client", "k8deployer")
        .Data(body)
        .Execute();

      hostnames_.insert(hostname);
    } catch(exception& ex) {
      LOG_ERROR << "Failed to provision hostname " << hostname << ": " << ex.what();
      throw;
    }

  }).get();
}

void DnsProvisionerVubercool::deleteHostname(const string &hostname)
{
    this->client_->ProcessWithPromise([&](Context& ctx) {
    auto url = "https://"s + config_.host + "/zone/" + hostname;

    LOG_DEBUG << "Deleting hostname " << hostname;

    try {
      auto reply = RequestBuilder(ctx)
        .Delete(url)
        .BasicAuthentication(config_.user, config_.passwd)
        .Header("X-Client", "k8deployer")
        .Execute();

      hostnames_.erase(hostname);
    } catch(exception& ex) {
      LOG_ERROR << "Failed to delete hostname " << hostname << ": " << ex.what();
      throw;
    }

  }).get();
}

} // ns

