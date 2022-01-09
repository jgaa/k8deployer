
#include <cstdlib>
#include "k8deployer/DnsMode.h"
#include "k8deployer/logging.h"

using namespace std;
using namespace std::string_literals;

namespace k8deployer {

void DnsMode::run()
{
    if (config_.command == "certbot-authenticator") {
        return authenticator();
    } else if (config_.command == "certbot-cleanup") {
        return cleanup();
    }

    throw runtime_error{"Unknown Dns command: "s + config_.command};
}

void DnsMode::authenticator()
{
    promise<bool> result_pr;

    assert(dns_);

    const string hostname = getEnv("CERTBOT_DOMAIN");
    const string validation = getEnv("CERTBOT_VALIDATION");

    LOG_INFO << "Settinng TXT entry '" << validation << "' to " << hostname;

    dns_->provisionHostname(hostname, validation, [&result_pr](bool success) {
        result_pr.set_value(success);
    }, false);

    if (!result_pr.get_future().get()) {
        throw runtime_error{"Failed to set txt entry to "s + hostname};
    }
}

void DnsMode::cleanup()
{
    promise<bool> result_pr;

    assert(dns_);

    const string hostname = getEnv("CERTBOT_DOMAIN");

    // TODO: Delete the validation node
    LOG_INFO << "NOT Deleting entry " << hostname;
}

void DnsMode::init()
{
    if (const auto dnsc = getenv("DNS_CONFIG")) {
        dns_ = DnsProvisioner::create(dnsc, client_->GetIoService());
        assert(dns_);
    }

    throw runtime_error{"No DNS_CONFIG set"};
}

string DnsMode::getEnv(string_view name)
{
    if (auto val = getenv(name.data())) {
        return {val};
    }

    throw runtime_error{"Missing "s + string{name}};
}

} // ns
