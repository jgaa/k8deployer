#include "k8deployer/logging.h"
#include "include/k8deployer/PortForward.h"

using namespace std;
using namespace string_literals;

namespace k8deployer {

uint16_t PortForward::ProcessCtx::nextId = 0;

void PortForward::start()
{
    namespace bp = boost::process;
    std::error_code ec;

    LOG_INFO << "Starting proxying over port " << getPort() << " to cluster " << ctx_.name;

    if (kubeFile_.empty()) {
        ctx_.child = make_unique<boost::process::child>(
                    bp::search_path("kubectl"), "proxy",
                    "-p", to_string(getPort()),
                    bp::std_in.close(),
                    bp::std_out > *ctx_.aps,
                    bp::std_err > *ctx_.ape, ec);
    } else {
        ctx_.child = make_unique<boost::process::child>(
                    bp::search_path("kubectl"), "proxy",
                    "-p", to_string(getPort()),
                    "--kubeconfig", kubeFile_,
                    bp::std_in.close(),
                    bp::std_out > *ctx_.aps,
                    bp::std_err > *ctx_.ape, ec);
    }

    if (ec) {
        LOG_ERROR << kubeFile_ << ": Failed to launch port-forwarding: " << ec;
        throw std::runtime_error("Failed to launch port-forwarding for: "s + ctx_.name);
    }

    fetchProcessOutput(*ctx_.aps, ctx_.stdout, false);
    fetchProcessOutput(*ctx_.ape, ctx_.stderr, true);
}

bool PortForward::waitForStarted()
{
    if (!ctx_.started.get_future().get()) {
        LOG_ERROR << "Failed to start port-forwarding on port " << getPort();
        return false;
    }

    return true;
}

PortForward::PortForward(boost::asio::io_context &ctx, const Config &cfg,
                         const string &kubeFile,
                         const string& name)
    : cfg_{cfg}, ctx_{ctx, cfg, kubeFile, name}, kubeFile_{kubeFile}
{

}

void PortForward::fetchProcessOutput(boost::process::async_pipe& ap,
                                     ProcessCtx::io_buf_t& buf,
                                     bool err)
{
    boost::asio::async_read_until(
                ap, buf, '\n',
                [this, &ap, &buf, err](const boost::system::error_code& ec, std::size_t /*size*/) {
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);

        if (err) {
            LOG_ERROR << ctx_.name << " kubectl said: " << line;
            if (ctx_.pending) {
                ctx_.pending = false;
                ctx_.started.set_value(false);
            }
        } else {
            LOG_DEBUG << ctx_.name << " kubectl said: " << line;
        }

        if (ec) {
            LOG_ERROR << ctx_.name << " IO error: " << ec.message();
            if (ctx_.pending) {
                ctx_.pending = false;
                ctx_.started.set_value(false);
            }
            return;
        }

        if (ctx_.pending) {
            ctx_.pending = false;
            ctx_.started.set_value(line.find("Starting to serve on") != string::npos);
        }

        fetchProcessOutput(ap, buf, err);
    });
}

} // ns
