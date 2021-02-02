#include "include/k8deployer/HostPathStorage.h"

#include "k8deployer/HostPathStorage.h"
#include "k8deployer/logging.h"
#include "k8deployer/Component.h"

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using namespace std;
using namespace string_literals;

namespace k8deployer {

HostPathStorage::HostPathStorage(const string &def)
{
    vector<string> args;
    boost::split(args, def, boost::is_any_of(":"));
    if (args.size() != 2) {
        throw runtime_error("There must be at least 2 hostpath args!");
    }

    hostPath_ = args[1];
}

k8api::PersistentVolume HostPathStorage::createNewVolume(const string &storageSize,
                                                    const Component &component)
{
    k8api::PersistentVolume pv;

    pv.spec.hostPath.emplace();
    pv.spec.hostPath->type = "DirectoryOrCreate";
    pv.spec.storageClassName = "manual";
    pv.spec.capacity["storage"] = storageSize;
    pv.spec.accessModes.push_back("ReadWriteOnce");
    pv.spec.persistentVolumeReclaimPolicy = "Delete";
    pv.metadata.annotations["pv.beta.kubernetes.io/gid"] = "1000";

    boost::filesystem::path nfsPath = pv.spec.nfs->path,
            path = hostPath_;
    path /= component.getNamespace();
    path /= component.name;

    if (Engine::instance().config().randomizePaths) {
        // Make sure we get a new dir each time we create a new volume!
        const auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
        path /= uuid;
    }

    pv.spec.hostPath->path = path.string();
    return pv;
}

} // ns

