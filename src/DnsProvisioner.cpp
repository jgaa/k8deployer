
#include <boost/fusion/adapted.hpp>

#include "k8deployer/Component.h"
#include "k8deployer/DnsProvisioner.h"
#include "k8deployer/DnsProvisionerVubercool.h"

namespace k8deployer {

struct DnsConfig {
  std::optional<DnsProvisionerVubercool::Config> vuberdns;
};

} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::DnsProvisionerVubercool::Config,
    (std::string, user)
    (std::string, passwd)
    (std::string, host)
    (size_t, retries)
    (size_t, retryDelaySecond)
);

BOOST_FUSION_ADAPT_STRUCT(k8deployer::DnsConfig,
    (std::optional<k8deployer::DnsProvisionerVubercool::Config>, vuberdns)
);

namespace k8deployer {

using namespace  std;

std::unique_ptr<DnsProvisioner> DnsProvisioner::create(const std::string &config,
                                                       boost::asio::io_service& ioservice)
{
    DnsConfig cfg;
    fileToObject(cfg, config, {});

    if (cfg.vuberdns) {
        return make_unique<DnsProvisionerVubercool>(*cfg.vuberdns, ioservice);
    }

    return {};
}

} // ns
