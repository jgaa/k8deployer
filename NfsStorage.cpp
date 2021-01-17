#include "k8deployer/NfsStorage.h"
#include "k8deployer/logging.h"
#include "k8deployer/Component.h"

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using namespace std;
using namespace string_literals;

namespace k8deployer {

NfsStorage::NfsStorage(const string &def)
{
    vector<string> args;
    boost::split(args, def, boost::is_any_of(":"));
    if (args.size() != 4) {
        throw runtime_error("There must be 4 nfs args!");
    }

    vs_.server = args[1];
    vs_.path = args[2];
    localPath_ = args[3];

    if (!boost::filesystem::is_directory(localPath_)) {
            LOG_ERROR << "Nfs path " << localPath_ << " is not a directory!";
        throw runtime_error("Nfs path must be a directory!");
    }
}

k8api::PersistentVolume NfsStorage::createNewVolume(const string &storageSize,
                                                    const Component &component)
{
    k8api::PersistentVolume pv;

    pv.spec.nfs = vs_;
    pv.spec.storageClassName = "nfs";
    pv.spec.capacity["storage"] = storageSize;
    pv.spec.accessModes.push_back("ReadWriteOnce");
    pv.spec.persistentVolumeReclaimPolicy = "Delete";
    pv.spec.mountOptions = {"hard"};

    boost::filesystem::path nfsPath = pv.spec.nfs->path,
            localPath = localPath_;
    nfsPath /= component.getNamespace();
    localPath /= component.getNamespace();

    nfsPath /= component.name;
    localPath /= component.name;

    // Make sure we get a new dir each time we create a new volume!
    const auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    nfsPath /= uuid;
    localPath /= uuid;

    boost::system::error_code ec;
    LOG_DEBUG << component.logName()
              << "Creating directory for NFS mount point: "
              << localPath << " --> " << vs_.server << ':' << nfsPath;

    if (!boost::filesystem::create_directories(localPath, ec)) {
        LOG_ERROR << "Failed to create NFS directory: " << localPath << ": " << ec;
        throw runtime_error(ec.message());
    }

    pv.spec.nfs->path = nfsPath.string();
    return pv;
}
}
