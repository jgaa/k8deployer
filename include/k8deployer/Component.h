#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <cassert>
#include <sstream>

#include <boost/fusion/adapted.hpp>

#include "restc-cpp/SerializeJson.h"

#include "k8deployer/Config.h"
#include "k8deployer/k8/Deployment.h"

namespace k8deployer {

class Cluster;
class Component;

enum class Kind {
    APP, // A placeholder that owns other components
    DEPLOYMENT,
    SERVICE
};

Kind toKind(const std::string& kind);

std::string toString(const Kind& kind);

template <typename T>
std::string toJson(const T& obj) {
    std::ostringstream out;
    restc_cpp::SerializeToJson(obj, out);
    return out.str();
}

using conf_t = std::map<std::string, std::string>;
using labels_t = std::vector<std::string>;

class K8Component {
public:
    struct CreateArgs {
        Component& component;
        Kind kind;
        std::string name;
        labels_t labels;
        conf_t args;
    };

    using ptr_t = std::shared_ptr<K8Component>;

    virtual ~K8Component() = default;
    virtual std::string name() const = 0;
    virtual void deploy() = 0;
    virtual void undeploy() = 0;
    virtual std::string id() = 0;
    virtual bool isThis(const std::string& ref) const = 0;

    static ptr_t create(const CreateArgs& ca);
};

/*! Tree of components to work with.
 *
 * The tree of components are read directly from json configuration
 */
class Component {
public:
    using childrens_t = std::deque<Component>;

    enum class State {
        PRE,
        CREATING,
        RUNNING,
        DONE, // No longer running in k8 because it's finished (task)
        FAILED
    };


    Component() = default;
    Component(Cluster& cluster)
        : cluster_{&cluster}
    {}
    Component(Component& parent, Cluster& cluster)
        : parent_{&parent}, cluster_{&cluster}
    {}

    // When to create a child, relative to the parent
    enum class ParentRelation {
        INDEPENDENT,
        BEFORE,
        AFTER
    };

    std::string name;
    std::string kind;
    labels_t labels;
    conf_t defaultArgs; // Added to args and childrens args, unless overridden
    conf_t args;
    std::string parentRelation; // Can be set from config

    // Can be populated by configuration, but normally we will do it
    k8api::Deployment deployment;

    // Related components owned by this; like volumes or configmaps or service.
    childrens_t children;

    // Call only on root node.
    void init();

    void deploy();

    Kind getKind() const noexcept {
        return kind_;
    };

    // Prefix for logging
    std::string logName() const noexcept;

    std::optional<bool> getBoolArg(const std::string& name) const;
    std::optional<std::string> getArg(const std::string& name) const;
    std::string getArg(const std::string& name, const std::string& defaultVal) const;
    int getIntArg(const std::string& name, int defaultVal) const;
    size_t getSizetArg(const std::string& name, size_t defaultVal) const;

    Cluster& cluster() noexcept {
        assert(cluster_);
        return *cluster_;
    }

private:
    void validate();
    void buildDependencies();
    void buildDeploymentDependencies();
    bool hasKindAsChild(Kind kind) const;
    void prepareDeploy();
    void initChildren();

    // Adds a child - does not call `init()`.
    Component& addChild(const std::string& name, Kind kind);
    conf_t mergeArgs() const;

    // Get a path to root, where the current node is first in the list
    std::vector<const Component *> getPathToRoot() const;

    State state_ = State::PRE; // From our logic
    std::string k8state_; // From the event-loop
    Component *parent_ = {};
    Cluster *cluster_ = {};
    K8Component::ptr_t component_;
    ParentRelation parentRelation_ = ParentRelation::AFTER;
    Kind kind_ = Kind::APP;
    conf_t effectiveArgs_;
};
} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::Component,
                          (std::string, name)
                          (std::string, kind)
                          (k8deployer::labels_t, labels)
                          (k8deployer::conf_t, defaultArgs)
                          (k8deployer::conf_t, args)
                          (std::string, parentRelation)
                          (k8deployer::Component::childrens_t, children)
                          );

