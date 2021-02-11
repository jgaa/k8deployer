#pragma once

#include <string>
#include <vector>
#include <map>

namespace k8deployer {

struct Config {
  std::string ns = "default";
  std::string definitionFile = "k8deployer.json";
  std::string command = "deploy";
  uint16_t localPort = 9000;
  std::vector<std::string> kubeconfigs;
  std::string storageEngine; // ex: nfs:nfshost:opt/nfs/k8:/mnt/nfs (type : host : mount-path : local path [where we can create subdirs for volumes])
  bool randomizePaths = false;
  bool useNetworkingBetaV1 = false;
  std::vector<std::string> rawVariables;

  std::map<std::string, std::string> variables;
};

} // ns
