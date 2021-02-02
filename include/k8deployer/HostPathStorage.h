#pragma once

#include <boost/filesystem.hpp>
#include "k8deployer/Storage.h"

namespace k8deployer {

class HostPathStorage: public Storage
{
public:
    HostPathStorage(const std::string& def);
    ~HostPathStorage() override = default;

    k8api::PersistentVolume createNewVolume(const std::string& storageSize,
                                            const Component& component) override;

private:
    boost::filesystem::path hostPath_;
};

} // ns

