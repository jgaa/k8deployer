
#include <algorithm>

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/Component.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/k8/k8api.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

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
        auto url = ca_.component.cluster().getUrl()
                + "/apis/apps/v1/namespaces/"
                + Engine::config().ns
                + "/deployments";

        Engine::client().Process([this, url](Context& ctx) {

            LOG_DEBUG << ca_.component.logName()
                      << "Sending Deployment "
                      << ca_.component.deployment.metadata.name;

            LOG_TRACE << "Payload: " << toJson(ca_.component.deployment);

            RequestBuilder builder{ctx};

            decltype (builder.Post(url).Execute()) reply;

            try {
                JsonFieldMapping mappings;
                mappings.entries.push_back({"template_", "template"});
                mappings.entries.push_back({"operator_", "operator"});
                reply = builder.Post(url)
                   .Data(ca_.component.deployment, &mappings)
                   .Execute();
            } catch(const RequestFailedWithErrorException& err) {
                LOG_WARN << ca_.component.logName()
                         << "Request failed: " << err.http_response.status_code
                         << ' ' << err.http_response.reason_phrase
                         << ": " << err.what();
            } catch(const std::exception& ex) {
                LOG_WARN << ca_.component.logName()
                         << "Request failed: " << ex.what();
            }

            if (reply) {
                LOG_DEBUG << ca_.component.logName()
                      << "Deployment gave response: "
                      << reply->GetResponseCode() << ' '
                      << reply->GetHttpResponse().reason_phrase;
            } else {
                // TODO: Deal with errors
            }

        });
    }
    void undeploy() override {

    }
    string id() override {
        return name();
    }
    bool isThis(const string &/*ref*/) const override {
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
    bool isThis(const string &/*ref*/) const override {
        return false;
    }

private:
    const CreateArgs ca_;
};

const map<string, Kind> kinds = {{"App", Kind::APP},
                                 {"Deployment", Kind::DEPLOYMENT},
                                 {"Service", Kind::SERVICE}
                                };
} // anonymous ns

Kind toKind(const string &kind)
{
    return kinds.at(kind);
}

string toString(const Kind &kind)
{
    for(const auto& [k, v] : kinds) {
        if (kind == v) {
            return k;
        }
    }
    throw runtime_error("Unknown Kind enum value.");
}

void Component::init_()
{
    kind_ = toKind(kind);
    effectiveArgs_ = mergeArgs();
    initChildren();
    validate();
    component_ = K8Component::create({*this, getKind(), name, labels, effectiveArgs_});
}

void RootComponent::init()
{
    init_();
}

void RootComponent::prepare()
{
    prepare_();
    buildTasks();
}

void Component::prepare_()
{
    // TODO: Walk the tree
    prepareComponent();

    for(auto& child : children) {
        child.prepare_();
    }
}

void RootComponent::deploy()
{
    //
}

void RootComponent::buildTasks()
{
    if (Engine::config().command == "deploy") {

        tasks_ = buildDeployTasks();
    }
}

string Component::logName() const noexcept
{
    return cluster_->name() + "/" + name + " ";
}

std::optional<bool> Component::getBoolArg(const string &name) const
{
    if (auto it = effectiveArgs_.find(name) ; it != effectiveArgs_.end()) {
        if (it->second == "true" || it->second == "yes" || it->second == "1") {
            return true;
        }

        if (it->second == "false" || it->second == "no" || it->second == "0") {
            return false;
        }

        throw runtime_error("Argument "s + name + " is not a boolean value (1|0|true|false|yes|no)");
    }

    return {};
}

std::optional<string> Component::getArg(const string &name) const
{
    if (auto it = effectiveArgs_.find(name) ; it != effectiveArgs_.end()) {
        return it->second;
    }

    return {};
}

string Component::getArg(const string &name, const string &defaultVal) const
{
    auto v = getArg(name);
    if (v) {
        return *v;
    }

    return defaultVal;
}

int Component::getIntArg(const string &name, int defaultVal) const
{
    auto v = getArg(name);
    if (v) {
        return stoi(*v);
    }

    return defaultVal;
}

size_t Component::getSizetArg(const string &name, size_t defaultVal) const
{
    auto v = getArg(name);
    if (v) {
        return stoll(*v);
    }

    return defaultVal;
}


void Component::validate()
{
    // TODO: Implement.
    // Add sanity checks, like; A Deployment owns it's Service, not the other way around...
}

void Component::buildDependencies()
{
    switch(getKind()) {
    case Kind::APP:
        ; // pass
        break;
    case Kind::DEPLOYMENT:
        buildDeploymentDependencies();
        break;
    case Kind::SERVICE:
        break;
    }

    for(auto& child: children) {
        child.buildDependencies();
    }
}

void Component::buildDeploymentDependencies()
{
    if (labels.empty()) {
        labels.emplace_back(name); // Use the name as selector
    }

    // A deployment normally needs a service
    const auto serviceEnabled = getBoolArg("service.enabled");
    if (!hasKindAsChild(Kind::SERVICE) && (serviceEnabled && *serviceEnabled)) {
        LOG_DEBUG << logName() << "Adding Service.";

        auto& service = addChild(name + "-svc", Kind::SERVICE);
        service.labels.push_back(labels.at(0)); // selector
        service.init_();
    }
}

bool Component::hasKindAsChild(Kind kind) const
{
    for(const auto& child : children) {
        if (child.getKind() == kind) {
            return true;
        }
    }

    return false;
}

void Component::prepareDeploy()
{
    deployment.metadata.name = name;

    // TODO: Fix and add labels
    if (labels.empty()) {
        labels.push_back(name);
    }
    const auto selector = getArg("selector",  labels.at(0));

    deployment.metadata.labels.try_emplace("app", selector);
    deployment.spec.selector.matchLabels.try_emplace("app", selector);

    if (deployment.spec.template_.metadata.name.empty()) {
        deployment.spec.template_.metadata.name = name;
    }
    deployment.spec.template_.metadata.labels.try_emplace("app", selector);

    if (auto replicas = getArg("replicas")) {
        deployment.spec.replicas = stoull(*replicas);
    }

    deployment.spec.selector.matchLabels.try_emplace("app", selector);
    deployment.spec.template_.metadata.labels.try_emplace("app", selector);

    if (deployment.spec.template_.spec.containers.empty()) {

        k8api::Container container;
        container.name = selector;
        container.image = getArg("image").value(); // TODO: Validate in validate()

        if (auto port = getArg("port")) {
            k8api::ContainerPort p;
            p.containerPort = static_cast<uint16_t>(stoul(*port));
            p.name = "default";
            if (auto v = getArg("protocol")) {
                p.protocol = *v;
            }
            container.ports.emplace_back(p);
        }

        deployment.spec.template_.spec.containers.push_back(move(container));
    }

}

void Component::prepareService()
{
    service.metadata.name = name;

    // TODO: Fix and add labels
    if (labels.empty()) {
        labels.push_back(name);
    }
    const auto selector = getArg("selector",  labels.at(0));

    service.metadata.labels.try_emplace("app", selector);
    service.spec.selector.try_emplace("app", selector);

    if (auto v = getArg("type")) {
        service.spec.type = *v;
    }

    if (service.spec.ports.empty() && parent_) {
        // Enumerate the owner objects ports and add each one

        // TODO: Deal with StatefulSet
        const auto& containers = parent_->deployment.spec.template_.spec.containers;
        size_t count = 0;
        for(const auto& cont: containers) {
            ++count;
            for(const auto& port : cont.ports) {
                k8api::ServicePort sp;
                sp.name = port.name;
                if (name.empty()) {
                    name = "port-"s + to_string(count);
                }
                sp.protocol = port.protocol;
                if (port.name.empty()) {
                    sp.port = port.containerPort;
                } else {
                    sp.targetPort = port.name;
                }
                sp.nodePort = getIntArg("nodePort", 0);
            }
        }
    }
}

void Component::initChildren()
{
    for(auto& child: children) {
        child.parent_ = this;
        child.cluster_ = cluster_;
        child.init_();
    }
}

Component::tasks_t Component::buildDeployDeploymentTasks()
{
    /* Deployment
     *  TODO: Deal with ConfigMap's and secrets
     *  TODO: Deal with volumes?
     *
     *  - Apply deployment
     *
     * There is no direct dependency between the Deployment and it's service
     */

    auto task = make_shared<Task>(*this, name, [&](Task& task) {
        task.setState(Task::TaskState::EXECUTING);
        component_->deploy();
        task.setState(Task::TaskState::WAITING);
    });
}

Component::tasks_t Component::buildDeployTasks()
{
    switch(getKind()) {
    case Kind::APP:
        return {};
    case Kind::DEPLOYMENT:
        return buildDeployDeploymentTasks();
    case Kind::SERVICE:
        return buildDeployServiceTasks();
    }
}

Component &Component::addChild(const string &name, Kind kind)
{
    auto& child = children.emplace_back(*this, *cluster_);

    child.name = name;
    child.kind = toString(kind);
    return child;
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
    if (ca.kind == Kind::DEPLOYMENT) {
        return make_shared<K8Deployment>(ca);
    }

    if (ca.kind == Kind::SERVICE) {
        return make_shared<K8Service>(ca);
    }

    LOG_ERROR << "Unknown kind in " << ca.name << ": " << toString(ca.kind);
    throw runtime_error("Unknown kind: "s + toString(ca.kind));
}

void k8deployer::Component::deployComponent()
{
    assert(component_);
    component_->deploy();
}

void k8deployer::Component::prepareComponent()
{
    buildDependencies();

    switch (getKind()) {
    case Kind::DEPLOYMENT:
        prepareDeploy();
        break;
    case Kind::SERVICE:
        prepareService();
        break;
    case Kind::APP:
        ; // Do nothing
        break;
    }
}

} // ns
