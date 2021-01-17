#pragma once

#include <memory>

#include "k8deployer/k8/k8api.h"

namespace k8deployer {

class Component;

class Storage
{
public:
    using ptr_t = std::unique_ptr<Storage>;
    Storage() = default;
    virtual ~Storage() = default;

    /*! Create a persistant volume declaration */
    virtual k8api::PersistentVolume createNewVolume(const std::string& storageSize,
                                                    const Component& component) = 0;

    static ptr_t create(const std::string& def);
};

} // ns

