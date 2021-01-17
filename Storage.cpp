#include "k8deployer/NfsStorage.h"
#include "k8deployer/logging.h"

using namespace std;
using namespace string_literals;

namespace k8deployer {

Storage::ptr_t Storage::create(const string &def)
{
    if (def.substr(0, 4) == "nfs:") {
        return make_unique<NfsStorage>(def);
    }

    LOG_ERROR << "I know nothing about this stoage configuration...: " << def;
    throw runtime_error("Unknown storage engine");
}


}
