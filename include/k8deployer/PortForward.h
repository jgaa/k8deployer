#pragma once

#include <array>
#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/process.hpp>

#include "k8deployer/Config.h"

namespace k8deployer {

class PortForward
{
public:
    struct ProcessCtx {
      using io_buf_t = boost::asio::streambuf;
      ProcessCtx(boost::asio::io_context& ctx, const Config& cfg, const std::string& kubeFile,
                 const std::string& name)
          : id{nextId++},
            port(id + cfg.localPort),
            name{name},
            ctx{ctx},
            aps{std::make_unique<boost::process::async_pipe>(ctx)},
            ape{std::make_unique<boost::process::async_pipe>(ctx)} {}

      const uint16_t id;
      const uint16_t port;
      const std::string name;
      boost::asio::io_context& ctx;
      std::unique_ptr<boost::process::async_pipe> aps;
      std::unique_ptr<boost::process::async_pipe> ape;
      std::unique_ptr<boost::process::child> child;
      io_buf_t stdout;
      io_buf_t stderr;
      static uint16_t nextId;
      std::promise<bool> started;
      bool pending = true;
    };

    PortForward(boost::asio::io_context& ctx, const Config& cfg,
                const std::string& kubeFile, const std::string& name);

    void start();

    // Returns false if port-forwarding failed
    bool waitForStarted();
    uint16_t getPort() const {
        return  ctx_.port;
    }

private:
    void fetchProcessOutput(boost::process::async_pipe& ap,
                            ProcessCtx::io_buf_t& buf,
                            bool err);

    const Config& cfg_;
    ProcessCtx ctx_;
    const std::string kubeFile_;
    const std::string name_;
};

} // ns
