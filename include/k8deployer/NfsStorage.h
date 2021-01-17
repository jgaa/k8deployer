#pragma once

#include <boost/filesystem.hpp>
#include "k8deployer/Storage.h"

namespace k8deployer {

class NfsStorage: public Storage
{
public:
    NfsStorage(const std::string& def);
    ~NfsStorage() override = default;

    k8api::PersistentVolume createNewVolume(const std::string& storageSize,
                                            const Component& component) override;

private:
    k8api::NFSVolumeSource vs_;
    boost::filesystem::path localPath_;
};

} // ns

