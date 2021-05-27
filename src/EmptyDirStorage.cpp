#include "k8deployer/EmptyDirStorage.h"
#include "k8deployer/logging.h"
#include "k8deployer/Component.h"

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using namespace std;
using namespace string_literals;

namespace k8deployer {

EmptyDirStorage::EmptyDirStorage(const string &def)
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


} // ns
