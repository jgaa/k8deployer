#pragma once

#include <boost/filesystem.hpp>
#include "k8deployer/Storage.h"

namespace k8deployer {

class EmptyDirStorage: public Storage
{
public:
    EmptyDirStorage(const std::string& def);
    ~EmptyDirStorage() override = default;

    k8api::PersistentVolume createNewVolume(const std::string& storageSize,
                                            const Component& component) override {
        return {};
    }

    Type type() const noexcept override {
        return Type::EMPTYDIR;
    }

private:
    k8api::NFSVolumeSource vs_;
    boost::filesystem::path localPath_;
};

} // ns

