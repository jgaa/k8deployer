#pragma once

#include <memory>

#include "k8deployer/k8/k8api.h"

namespace k8deployer {

class Component;

class Storage
{
public:
    enum class Type {
        GENERIC,
        EMPTYDIR
    };

    using ptr_t = std::unique_ptr<Storage>;
    Storage() = default;
    virtual ~Storage() = default;

    /*! Create a persistant volume declaration */
    virtual k8api::PersistentVolume createNewVolume(const std::string& storageSize,
                                                    const Component& component) = 0;

    virtual Type type() const noexcept {
        return Type::GENERIC;
    }

    static ptr_t create(const std::string& def);
};

} // ns

