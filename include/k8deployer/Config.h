#pragma once

#include <string>
#include <vector>

namespace k8deployer {

struct Config {
  std::string ns = "default";
  std::string definitionFile = "k8deployer.json";
  std::string command = "deploy";
  uint16_t localPort = 9000;
  std::vector<std::string> kubeconfigs;
};

} // ns
