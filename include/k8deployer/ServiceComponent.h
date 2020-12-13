#pragma once

#include "k8deployer/Component.h"

namespace k8deployer {

class ServiceComponent : public Component
{
public:
    ServiceComponent(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : Component(parent, cluster, data)
    {}
};

} // ns



