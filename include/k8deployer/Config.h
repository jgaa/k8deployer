#pragma once

#include <string>
#include <vector>
#include <map>
#include <regex>

namespace k8deployer {

struct Config {
  std::string ns = "default";
  std::string definitionFile = "k8deployer.json";
  std::string command = "deploy";
  std::vector<std::string> kubeconfigs;
  std::string storageEngine; // ex: nfs:nfshost:opt/nfs/k8:/mnt/nfs (type : host : mount-path : local path [where we can create subdirs for volumes])
  bool randomizePaths = false;
  bool useNetworkingBetaV1 = false;
  std::vector<std::string> rawVariables;
  std::string excludeFilter;
  std::string enabledFilter;
  std::string includeFilter = ".*";
  std::map<std::string, std::string> variables;
  bool autoMaintainNamespace = false;
  std::string dotfile = "k8deployer.dot";
  bool skipDependencyInitContainers = false;
  std::vector<std::string> variants;
  std::vector<std::string> removeEnvVars;
  bool useClusterNameAsNamespace = false;
  bool useLoadBalancerIp = false;
  std::string dnsServerConfig;
  bool useFirstPartOfKubeConfigAsClusterName = true; // Us thye part before the first dot
  std::string logDir;
  std::string logViewer;
  bool wipeLogDir = false;
  std::string webBrowser;
  std::string pvcStorageClassName;
  bool ignoreResourceLimits = false;
};

} // ns
