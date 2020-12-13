
#include <map>
#include <algorithm>

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/Component.h"
#include "k8deployer/AppComponent.h"
#include "k8deployer/DeploymentComponent.h"
#include "k8deployer/ServiceComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/k8/k8api.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

namespace {
//class K8Deployment : public K8Component {
//public:
//    K8Deployment(CreateArgs ca)
//        : ca_{move(ca)}
//    {
//    }

//    string name() const override {
//        return ca_.name;
//    }

//    void deploy() override {
//        auto url = ca_.component.cluster().getUrl()
//                + "/apis/apps/v1/namespaces/"
//                + Engine::config().ns
//                + "/deployments";

//        Engine::client().Process([this, url](Context& ctx) {

//            LOG_DEBUG << ca_.component.logName()
//                      << "Sending Deployment "
//                      << ca_.component.deployment.metadata.name;

//            LOG_TRACE << "Payload: " << toJson(ca_.component.deployment);

//            RequestBuilder builder{ctx};

//            decltype (builder.Post(url).Execute()) reply;

//            try {
//                JsonFieldMapping mappings;
//                mappings.entries.push_back({"template_", "template"});
//                mappings.entries.push_back({"operator_", "operator"});
//                reply = builder.Post(url)
//                   .Data(ca_.component.deployment, &mappings)
//                   .Execute();
//            } catch(const RequestFailedWithErrorException& err) {
//                LOG_WARN << ca_.component.logName()
//                         << "Request failed: " << err.http_response.status_code
//                         << ' ' << err.http_response.reason_phrase
//                         << ": " << err.what();
//            } catch(const std::exception& ex) {
//                LOG_WARN << ca_.component.logName()
//                         << "Request failed: " << ex.what();
//            }

//            if (reply) {
//                LOG_DEBUG << ca_.component.logName()
//                      << "Deployment gave response: "
//                      << reply->GetResponseCode() << ' '
//                      << reply->GetHttpResponse().reason_phrase;
//            } else {
//                // TODO: Deal with errors
//            }

//        });
//    }
//    void undeploy() override {

//    }
//    string id() override {
//        return name();
//    }
//    bool isThis(const string &/*ref*/) const override {
//        return false;
//    }


//private:
//    const CreateArgs ca_;
//};

//class K8Service : public K8Component {
//public:
//    K8Service(CreateArgs ca)
//        : ca_{move(ca)}
//    {
//    }

//    string name() const override {
//        return ca_.name;
//    }
//    void deploy() override {

//    }
//    void undeploy() override {

//    }
//    string id() override {
//        return name();
//    }
//    bool isThis(const string &/*ref*/) const override {
//        return false;
//    }

//private:
//    const CreateArgs ca_;
//};

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

void Component::init()
{
    setState(State::CREATING);
    effectiveArgs_ = mergeArgs();
    initChildren();
    validate();
}

//void RootComponent::init()
//{
//    init_();
//}

//void RootComponent::prepare()
//{
//    prepare_();
//    buildTasks();
//}

//void Component::prepare_()
//{
//    // TODO: Walk the tree
//    prepareComponent();

//    for(auto& child : children) {
//        child.prepare_();
//    }
//}

//void RootComponent::deploy()
//{
//    //
//}

//void RootComponent::buildTasks()
//{
//    if (Engine::config().command == "deploy") {

//        tasks_ = buildDeployTasks();
//    }
//}

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

Component::ptr_t Component::populateTree(const ComponentDataDef &def, Cluster &cluster)
{
    auto root = populate(def, cluster, {});
    root->init();
    return root;
}

std::future<void> Component::prepareDeploy()
{
    for(auto& child : children_) {
        child->prepareDeploy();
    }

    return dummyReturnFuture();
}

std::future<void> Component::deploy()
{

    // Built list of tasks
    tasks_ = make_unique<tasks_t>();
    addTasks(*tasks_);

    // TODO: Set dependencies

    // Execute via asio's executor
    cluster().client().GetIoService().post([self = weak_from_this()] {
       if (auto component = self.lock()) {
           component->runTasks();
       }
    });

    executionPromise_ = make_unique<promise<void>>();
    return executionPromise_->get_future();
}

void Component::onEvent(const std::shared_ptr<k8api::Event>& event)
{
    if (tasks_) {
        cluster_->client().GetIoService().post([event, self = weak_from_this()] {
            if (auto component = self.lock()) {
                component->processEvent(*event);
            }
        });
    }
}

labels_t::value_type Component::getSelector()
{
    if (auto selector = labels.find("app"); selector != labels.end()) {
        return *selector;
    }

    return {"app", name};
}

void Component::processEvent(const k8api::Event& event)
{
    assert(tasks_);
    bool changed = false;
    for(auto& task : *tasks_) {
        changed = task->onEvent(event) ? true : changed;
    }

    if (changed) {
        cluster_->client().GetIoService().post([event, self = weak_from_this()] {
            if (auto component = self.lock()) {
                component->runTasks();
            }
        });
    }
}

void Component::runTasks() {
    // Re-run the loop as long as any task changes it's state
    while(cluster_->isExecuting() && ! isDone()) {
        LOG_TRACE << logName() << "runTasks: Iterating over tasks";
        bool again = false;
        bool allDone = true;
        assert(tasks_);
        for(auto task : *tasks_) {
            again = task->evaluate() ? true : again;
            if (task->state() == Task::TaskState::READY) {
                task->execute();
                again = true;
            }

            allDone = task->isDone() ? allDone : false;
        }

        if (allDone) {
            LOG_INFO << logName() << "All DONE.";
            setState(State::DONE);
            if (executionPromise_) {
                executionPromise_->set_value();
                executionPromise_.reset();
            }
            return;
        }

        if (!again) {
            // TODO: Add timer so we can time out if we don't catch or get
            // events to move the states to DONE.

            LOG_TRACE << logName() << "runTasks: Finished iterations for now ...";
            return;
        }
    }

    // TODO: Deal with incomplete state if we are not finished

    LOG_TRACE << logName() << "runTasks: Finished iterations. Cluster isExecuring: "
              << (cluster_->isExecuting() ? "true" : "false")
              << ", isDone: " << (isDone() ? "true" : "false");

}

void Component::addTasks(Component::tasks_t& tasks)
{
    setState(State::RUNNING);
    for(auto& child : children_) {
        child->addTasks(tasks);
    }
}

Component::ptr_t Component::createComponent(const ComponentDataDef &def,
                                            const Component::ptr_t& parent,
                                            Cluster& cluster)
{
    auto kind = toKind(def.kind);
    switch(kind) {
    case Kind::APP:
        return make_shared<AppComponent>(parent, cluster, def);
    case Kind::DEPLOYMENT:
        return make_shared<DeploymentComponent>(parent, cluster, def);
    case Kind::SERVICE:
        return make_shared<ServiceComponent>(parent, cluster, def);
    }

    throw runtime_error("Unknown kind");
}

Component::ptr_t Component::populate(const ComponentDataDef &def,
                         Cluster &cluster,
                         const Component::ptr_t &parent)
{
    auto component = createComponent(def, parent, cluster);
    if (def.parentRelation == "before") {
        component->parentRelation_ = ParentRelation::BEFORE;
    } else if (def.parentRelation == "after") {
        component->parentRelation_ = ParentRelation::AFTER;
    } else if (def.parentRelation == "independent") {
        component->parentRelation_ = ParentRelation::INDEPENDENT;
    }

    for(auto& childDef : def.children) {
        auto child = createComponent(childDef, component, cluster);
        component->children_.push_back(child);
        populate(childDef, cluster, child);
    }

    return component;
}

std::future<void> Component::dummyReturnFuture() const
{
    std::promise<void> promise;
    auto future = promise.get_future();
    promise.set_value();
    return future;
}

void Component::validate()
{
    // TODO: Implement.
    // Add sanity checks, like; A Deployment owns it's Service, not the other way around...
}

//void Component::buildDependencies()
//{
////    switch(getKind()) {
////    case Kind::APP:
////        ; // pass
////        break;
////    case Kind::DEPLOYMENT:
////        buildDeploymentDependencies();
////        break;
////    case Kind::SERVICE:
////        break;
////    }

//    for(auto& child: children_) {
//        child->buildDependencies();
//    }
//}

//void Component::buildDeploymentDependencies()
//{
//    if (labels.empty()) {
//        labels.emplace_back(name); // Use the name as selector
//    }

//    // A deployment normally needs a service
//    const auto serviceEnabled = getBoolArg("service.enabled");
//    if (!hasKindAsChild(Kind::SERVICE) && (serviceEnabled && *serviceEnabled)) {
//        LOG_DEBUG << logName() << "Adding Service.";

//        auto& service = addChild(name + "-svc", Kind::SERVICE);
//        service.labels.push_back(labels.at(0)); // selector
//        service.init_();
//    }
//}

bool Component::hasKindAsChild(Kind kind) const
{
    for(const auto& child : children_) {
        if (child->getKind() == kind) {
            return true;
        }
    }

    return false;
}

//void Component::prepareDeploy()
//{
//    deployment.metadata.name = name;

//    // TODO: Fix and add labels
//    if (labels.empty()) {
//        labels.push_back(name);
//    }
//    const auto selector = getArg("selector",  labels.at(0));

//    deployment.metadata.labels.try_emplace("app", selector);
//    deployment.spec.selector.matchLabels.try_emplace("app", selector);

//    if (deployment.spec.template_.metadata.name.empty()) {
//        deployment.spec.template_.metadata.name = name;
//    }
//    deployment.spec.template_.metadata.labels.try_emplace("app", selector);

//    if (auto replicas = getArg("replicas")) {
//        deployment.spec.replicas = stoull(*replicas);
//    }

//    deployment.spec.selector.matchLabels.try_emplace("app", selector);
//    deployment.spec.template_.metadata.labels.try_emplace("app", selector);

//    if (deployment.spec.template_.spec.containers.empty()) {

//        k8api::Container container;
//        container.name = selector;
//        container.image = getArg("image").value(); // TODO: Validate in validate()

//        if (auto port = getArg("port")) {
//            k8api::ContainerPort p;
//            p.containerPort = static_cast<uint16_t>(stoul(*port));
//            p.name = "default";
//            if (auto v = getArg("protocol")) {
//                p.protocol = *v;
//            }
//            container.ports.emplace_back(p);
//        }

//        deployment.spec.template_.spec.containers.push_back(move(container));
//    }

//}

//void Component::prepareService()
//{
//    service.metadata.name = name;

//    // TODO: Fix and add labels
//    if (labels.empty()) {
//        labels.push_back(name);
//    }
//    const auto selector = getArg("selector",  labels.at(0));

//    service.metadata.labels.try_emplace("app", selector);
//    service.spec.selector.try_emplace("app", selector);

//    if (auto v = getArg("type")) {
//        service.spec.type = *v;
//    }

//    if (service.spec.ports.empty() && parent_) {
//        // Enumerate the owner objects ports and add each one

//        // TODO: Deal with StatefulSet
//        const auto& containers = parent_->deployment.spec.template_.spec.containers;
//        size_t count = 0;
//        for(const auto& cont: containers) {
//            ++count;
//            for(const auto& port : cont.ports) {
//                k8api::ServicePort sp;
//                sp.name = port.name;
//                if (name.empty()) {
//                    name = "port-"s + to_string(count);
//                }
//                sp.protocol = port.protocol;
//                if (port.name.empty()) {
//                    sp.port = port.containerPort;
//                } else {
//                    sp.targetPort = port.name;
//                }
//                sp.nodePort = getIntArg("nodePort", 0);
//            }
//        }
//    }
//}

void Component::initChildren()
{
    for(auto& child: children_) {
        child->init();
    }
}

Component::ptr_t Component::addChild(const string &name, Kind kind, const labels_t &labels)
{
    ComponentDataDef def;
    def.labels = labels;
    def.name = name;
    def.kind = toString(kind);
    assert(cluster_);
    auto component = createComponent(def, shared_from_this(), *cluster_);
    component->init();
    children_.push_back(component);
    return component;
}

//Component::tasks_t Component::buildDeployDeploymentTasks()
//{
//    /* Deployment
//     *  TODO: Deal with ConfigMap's and secrets
//     *  TODO: Deal with volumes?
//     *
//     *  - Apply deployment
//     *
//     * There is no direct dependency between the Deployment and it's service
//     */

//    auto task = make_shared<Task>(*this, name, [&](Task& task) {

//        if (task.state() == Task::TaskState::READY)
//        task.setState(Task::TaskState::EXECUTING);
//        component_->deploy();
//        task.setState(Task::TaskState::WAITING);
//    });
//}

//Component::tasks_t Component::buildDeployTasks()
//{
//    switch(getKind()) {
//    case Kind::APP:
//        return {};
//    case Kind::DEPLOYMENT:
//        return buildDeployDeploymentTasks();
//    case Kind::SERVICE:
//        return buildDeployServiceTasks();
//    }
//}

//Component::ptr_t &Component::addChild(const string &name, Kind kind, const labels_t& labels)
//{
////    auto& child = children.emplace_back(*this, *cluster_);

////    child.name = name;
////    child.kind = toString(kind);
////    return child;
//}

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
    for(auto p = this; p != nullptr; p = parentPtr()) {
        path.push_back(p);
    }
    return path;
}

const Component *Component::parentPtr() const
{
    if (auto parent = parent_.lock()) {
        return parent.get();
    }

    return {};
}

void Component::Task::setState(Component::Task::TaskState state)
{
    LOG_TRACE << component().logName() << " Task " << name() << " change state from "
              << toString(state_) << " to " << toString(state);
    state_ = state;
}

bool Component::Task::evaluate()
{
    bool changed = false;

    if (state_ == TaskState::PRE) {
        changed = true;
        state_ = TaskState::BLOCKED;
    }

    if (state_ == TaskState::BLOCKED) {
        bool blocked = false;
        for(const auto& dep : dependencies_) {

            // If any dependencies is not DONE, we are still blocked.
            if (auto dt = dep.lock()) {
                blocked = dt->state() == TaskState::DONE ? blocked : true;

                if (dt->state() >= TaskState::ABORTED) {
                    // If a dependency failed, we abort.
                    // TODO: Deal with roll-backs
                    setState(TaskState::DEPENDENCY_FAILED);
                    return true;
                }
            }
        }

        if (!blocked) {
            setState(TaskState::READY);
            changed = true;
        }
    } // if BLOCKED

    return changed;
}

const string &Component::Task::toString(const Component::Task::TaskState &state) {
    static const array<string, 9> names = { "PRE",
                                            "BLOCKED",
                                            "READY",
                                            "EXECUTING",
                                            "WAITING", // Waiting for events to update it's status
                                            "DONE",
                                            "ABORTED",
                                            "FAILED",
                                            "DEPENDENCY_FAILED"};

    return names.at(static_cast<size_t>(state));
}

//K8Component::ptr_t K8Component::create(const K8Component::CreateArgs &ca)
//{
//    if (ca.kind == Kind::DEPLOYMENT) {
//        return make_shared<K8Deployment>(ca);
//    }

//    if (ca.kind == Kind::SERVICE) {
//        return make_shared<K8Service>(ca);
//    }

//    LOG_ERROR << "Unknown kind in " << ca.name << ": " << toString(ca.kind);
//    throw runtime_error("Unknown kind: "s + toString(ca.kind));
//}

//void k8deployer::Component::deployComponent()
//{
//    assert(component_);
//    component_->deploy();
//}

//void k8deployer::Component::prepareComponent()
//{
//    buildDependencies();

//    switch (getKind()) {
//    case Kind::DEPLOYMENT:
//        prepareDeploy();
//        break;
//    case Kind::SERVICE:
//        prepareService();
//        break;
//    case Kind::APP:
//        ; // Do nothing
//        break;
//    }
//}

} // ns
