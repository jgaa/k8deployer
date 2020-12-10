#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <deque>

#include <boost/fusion/adapted.hpp>

#include "k8deployer/Config.h"

namespace k8deployer {

using conf_t = std::map<std::string, std::string>;
using labels_t = std::vector<std::string>;

class K8Component {
public:
    struct CreateArgs {
        std::string kind;
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

    // Related components owned by this; like volumes or configmaps or service.
    childrens_t children;

    // Call only on root node.
    void init();

private:
    void initChildren();
    conf_t mergeArgs() const;

    // Get a path to root, where the current node is first in the list
    std::vector<const Component *> getPathToRoot() const;

    State state_ = State::PRE; // From our logic
    std::string k8state_; // From the event-loop
    Component *parent_ = {};
    K8Component::ptr_t component_;
    ParentRelation parentRelation_ = ParentRelation::AFTER;
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

