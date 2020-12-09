
#include <algorithm>
#include "k8deployer/logging.h"
#include "include/k8deployer/Component.h"

using namespace std;
using namespace string_literals;

namespace k8deployer {

namespace {
class K8Deployment : public K8Component {
public:
    K8Deployment(CreateArgs ca)
        : ca_{move(ca)}
    {
    }

    string name() const override {
        return ca_.name;
    }
    void deploy() override {

    }
    void undeploy() override {

    }
    string id() override {
        return name();
    }
    bool isThis(const string &ref) const override {
        return false;
    }

private:
    const CreateArgs ca_;
};

class K8Service : public K8Component {
public:
    K8Service(CreateArgs ca)
        : ca_{move(ca)}
    {
    }

    string name() const override {
        return ca_.name;
    }
    void deploy() override {

    }
    void undeploy() override {

    }
    string id() override {
        return name();
    }
    bool isThis(const string &ref) const override {
        return false;
    }

private:
    const CreateArgs ca_;
};
} // anonymous ns

void Component::init()
{
    initChildren();
    component_ = K8Component::create({kind, name, tags, mergeArgs()});
}

void Component::initChildren()
{
    for(auto& child: children) {
        child.parent_ = this;
        child.init();
    }
}

conf_t Component::mergeArgs() const
{
    conf_t merged = args;
    for(const auto node: getPathToRoot()) {
        for(const auto& [k, v]: node->defaultArgs) {
            merged.try_emplace(k, v);
        }
    }

    return merged;
}

std::vector<const Component*> Component::getPathToRoot() const
{
    std::vector<const Component *> path;
    for(auto p = this; p != nullptr; p = p->parent_) {
        path.push_back(p);
    }
    return path;
}

K8Component::ptr_t K8Component::create(const K8Component::CreateArgs &ca)
{
    if (ca.kind == "Deployment") {
        return make_shared<K8Deployment>(ca);
    }

    if (ca.kind == "Service") {
        return make_shared<K8Service>(ca);
    }

    LOG_ERROR << "Unknown kind in " << ca.name << ": " << ca.kind;
    throw runtime_error("Unknown kind: "s + ca.kind);
}

}
