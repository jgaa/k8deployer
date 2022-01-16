

#include "k8deployer/Config.h"
#include "k8deployer/logging.h"

using namespace std;
using namespace string_literals;

namespace k8deployer {

Config::Config()
{
    if (auto d = getenv("K8DEPLOYER_DIR")) {
       dir = d;
       return;
    }

    if (auto d = getenv("HOME")) {
       dir = d;
       dir /= ".k8deployer";
       return;
    }

    LOG_DEBUG << "Unable to deduce k8deployers home-directory";
}

} // ns
