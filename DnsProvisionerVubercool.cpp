
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

DnsProvisionerVubercool::DnsProvisionerVubercool(const DnsProvisionerVubercool::Config &cfg,
                                                 boost::asio::io_service& ioservice)
  : config_{cfg}
{
    client_ = RestClient::Create(ioservice);
}

void DnsProvisionerVubercool::provisionHostname(const std::string &hostname,
                                                const std::vector<std::string> &ipv4,
                                                const std::vector<std::string> &ipv6,
                                                const cb_t& onDone) {
    client_->Process([this, hostname, ipv4, ipv6, onDone](Context& ctx) {
        auto url = "https://"s + config_.host + "/zone/" + hostname;

        VubercoolReq body;
        body.a = ipv4;
        body.aaaa = ipv6;

        for(size_t i = 0; i < config_.retries; ++i) {
            if (i /* is retry */) {
                ctx.Sleep(std::chrono::seconds(config_.retryDelaySecond));
            }

            if (!addHostname(hostname)) {
                LOG_DEBUG << "Hostname " << hostname << " was/is already provisioned. Skipping...";
                onDone(true);
                return;
            }

            try {
                LOG_DEBUG << "Provisioning hostname " << hostname;
                auto reply = RequestBuilder(ctx)
                  .Patch(url)
                  .BasicAuthentication(config_.user, config_.passwd)
                  .Header("X-Client", "k8deployer")
                  .Data(body)
                  .Execute();

                onDone(true);
                return;
            } catch(exception& ex) {
                LOG_WARN << "Failed to provision hostname " << hostname << ": " << ex.what();
            }

            deleteHostname(hostname); // was not provisioned after all
        }

        LOG_ERROR << "Failed to provision hostname (no more retries left): " << hostname;
        onDone(false);
    });
}

void DnsProvisionerVubercool::deleteHostname(const string &hostname, const cb_t& onDone)
{
    client_->Process([this, hostname, onDone](Context& ctx) {
        auto url = "https://"s + config_.host + "/zone/" + hostname;

        for(size_t i = 0; i < config_.retries; ++i) {
            if (i /* is retry */) {
                ctx.Sleep(std::chrono::seconds(config_.retryDelaySecond));
            }

            LOG_DEBUG << "Deleting hostname " << hostname;

            try {
                auto reply = RequestBuilder(ctx)
                  .Delete(url)
                  .BasicAuthentication(config_.user, config_.passwd)
                  .Header("X-Client", "k8deployer")
                  .Execute();

                deleteHostname(hostname);
                onDone(true);
            } catch(const restc_cpp::HttpNotFoundException& ) {
                deleteHostname(hostname);
                LOG_DEBUG << "Hostname " << hostname << " was already deleted.";
                onDone(true);
            } catch(exception& ex) {
                LOG_WARN << "Failed to delete hostname " << hostname << ": " << ex.what();
            }
        }

        LOG_ERROR << "Failed to delete hostname (no more retries left): " << hostname;
        onDone(false);
    });
}

namespace {
  pair<mutex&, set<string>&> getHostStuff() {
     static mutex m;
     static set<string> s;

     return {m, s};
  }
}

bool DnsProvisionerVubercool::addHostname(const string &hostname)
{
    auto [m, s] = getHostStuff();
    lock_guard<mutex> lock(m);
    auto [_, inserted] = s.insert(hostname);
    return inserted;
}

void DnsProvisionerVubercool::deleteHostname(const string &hostname)
{
    auto [m, s] = getHostStuff();
    lock_guard<mutex> lock(m);
    s.erase(hostname);
}

} // ns

