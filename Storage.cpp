#include "k8deployer/NfsStorage.h"
#include "k8deployer/HostPathStorage.h"
#include "k8deployer/logging.h"

using namespace std;
using namespace string_literals;

namespace k8deployer {

Storage::ptr_t Storage::create(const string &def)
{
    static const string nfs = "nfs:";
    if (def.substr(0, nfs.size()) == nfs) {
        return make_unique<NfsStorage>(def);
    }

    static const string hostpath = "hostpath:";
    if (def.substr(0, hostpath.size()) == hostpath) {
        return make_unique<HostPathStorage>(def);
    }

    LOG_ERROR << "I know nothing about this stoage configuration...: " << def;
    throw runtime_error("Unknown storage engine");
}


}
