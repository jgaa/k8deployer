
#include <map>
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/logging.h"
#include "k8deployer/Component.h"
#include "k8deployer/AppComponent.h"
#include "k8deployer/JobComponent.h"
#include "k8deployer/DeploymentComponent.h"
#include "k8deployer/ServiceComponent.h"
#include "k8deployer/ConfigMapComponent.h"
#include "k8deployer/SecretComponent.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/k8/k8api.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

namespace {

const map<string, Kind> kinds = {{"App", Kind::APP},
                                 {"Job", Kind::JOB},
                                 {"Deployment", Kind::DEPLOYMENT},
                                 {"Service", Kind::SERVICE},
                                 {"ConfigMap", Kind::CONFIGMAP},
                                 {"Secret", Kind::SECRET}
                                };
} // anonymous ns

// https://stackoverflow.com/questions/18816126/c-read-the-whole-file-in-buffer
string slurp (const string& path) {
  ostringstream buf;
  ifstream input (path.c_str());
  buf << input.rdbuf();
  return buf.str();
}


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

std::string Base64Encode(const std::string &in) {
    // Silence the cursed clang-tidy...
    constexpr auto magic_4 = 4;
    constexpr auto magic_6 = 6;
    constexpr auto magic_8 = 8;
    constexpr auto magic_3f = 0x3F;

    static const string alphabeth {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
    string out;

    int val = 0;
    int valb = -magic_6;
    for (const uint8_t c : in) {
        val = (val<<magic_8) + c;
        valb += magic_8;
        while (valb>=0) {
            out.push_back(alphabeth[(val>>valb)&magic_3f]);
            valb-=magic_6;
        }
    }
    if (valb>-magic_6) out.push_back(alphabeth[((val<<magic_8)>>(valb+magic_8))&magic_3f]);
    while (out.size()%magic_4) out.push_back('=');
    return out;
}

string Component::logName() const noexcept
{
    return cluster_->name() + "/" + toString(kind_) + "/" + name + " ";
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

k8api::string_list_t Component::getArgAsStringList(const string &values, const string &defaultVal) const
{
    auto val = getArg(values, defaultVal);
    k8api::string_list_t rval;

    enum class State {
        SKIPPING,
        IN_STRING,
        IN_QUOTED_STRING
    };

    auto state = State::SKIPPING;
    string value;

    for (const char ch : val) {
        switch (state) {
            case State::SKIPPING:
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                continue;
            }
            if (ch == '\'') {
                state = State::IN_QUOTED_STRING;
                continue;
            }
            state = State::IN_STRING;
            break;
        case State::IN_STRING:
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                rval.push_back(value);
                value.clear();
                state = State::SKIPPING;
                continue;
            }
        case State::IN_QUOTED_STRING:
            if (ch == '\'') {
                rval.push_back(value);
                value.clear();
                state = State::SKIPPING;
                continue;
            }
        }

        value += ch;
    }

    if (!value.empty()) {
        rval.push_back(value);
    }

//    boost::split(list, val, boost::is_any_of(" "));

//    k8api::string_list_t rval;
//    for(const auto& segment : list) {
//        if (segment.empty()) {
//            continue;
//        }
//        rval.push_back(segment);
//    }

    return rval;
}

k8api::env_vars_t Component::getArgAsEnvList(const string &values, const string &defaultVal) const
{
    k8api::env_vars_t rval;
    auto list = getArgAsStringList(values, defaultVal);

    for(const auto& v : list) {
        k8api::KeyValue ev;
        if (auto pos = v.find('='); pos != string::npos && pos < v.size()) {
            ev.name = v.substr(0, pos);
            ev.value = v.substr(pos +1);
        } else {
            ev.name = v; // Just an empty variable
        }

        if (!ev.name.empty()) {
            rval.push_back(ev);
        }
    }

    return rval;
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
    return execute([this](tasks_t& tasks) {
        addDeploymentTasks(tasks);
    });
}

std::future<void> Component::remove()
{
    reverseDependencies_ = true;
    return execute([this](tasks_t& tasks) {
        addRemovementTasks(tasks);
    });
}

std::future<void> Component::execute(std::function<void(tasks_t&)> fn)
{
    // Built list of tasks
    tasks_ = make_unique<tasks_t>();
    fn(*tasks_);

    // TODO: Add task dependencies based on component dependencies (depends property)

    // Set dependencies
    for(auto& task : *tasks_) {
        auto relation = task->component().parentRelation();
        if (reverseDependencies_) {
            if (relation == ParentRelation::AFTER) {
                relation = ParentRelation::BEFORE;
            } else if (relation == ParentRelation::BEFORE) {
                relation = ParentRelation::AFTER;
            }
        }

        switch(relation) {
        case ParentRelation::AFTER:
            // The task depend on parent task(s)
            if (auto parent = task->component().parent_.lock()) {
                for(auto ptask : *tasks_) {
                    if (&ptask->component() == parent.get()) {
                        LOG_TRACE << logName() << "Task " << task->name() << " depends on " << ptask->name();
                        task->addDependency(ptask->weak_from_this());
                    }
                }
            }
            break;
        case ParentRelation::BEFORE:
            // The parent's tasks depend on the task(s)
            if (auto parent = task->component().parent_.lock()) {
                for(auto ptask : *tasks_) {
                    if (&ptask->component() == parent.get()) {
                        LOG_TRACE << logName() << "Task " << ptask->name() << " depends on " << task->name();
                        ptask->addDependency(task->weak_from_this());
                    }
                }
            }
            break;
        case ParentRelation::INDEPENDENT:
            ; // Don't matter
        }
    }

    // TODO: Check for circular dependencies

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

void Component::scheduleRunTasks()
{
    if (auto parent = parent_.lock()) {
        return parent->scheduleRunTasks();
    }

    cluster().client().GetIoService().post([self = weak_from_this()] {
       if (auto component = self.lock()) {
           component->runTasks();
       }
    });
}

void Component::processEvent(const k8api::Event& event)
{
    assert(tasks_);
    bool changed = false;
    for(auto& task : *tasks_) {
        changed = task->onEvent(event) ? true : changed;
        if (changed) {
            LOG_TRACE << logName() << " Task " << task->name() << " changed state. Will schedule a re-run of the tasks.";
        }
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

    LOG_TRACE << logName() << "runTasks: Finished iterations. Cluster isExecuting: "
              << (cluster_->isExecuting() ? "true" : "false")
              << ", isDone: " << (isDone() ? "true" : "false");

}

void Component::setState(Component::State state)
{
    if (state == State::DONE) {
        LOG_INFO << logName() << "Done.";
    }

    if (state == State::FAILED) {
        LOG_WARN << logName() << "Failed.";
    }

    state_ = state;
}

void Component::forAllComponents(const std::function<void (Component &)>& fn)
{
    auto root = this;
    while (true) {
        if (auto p = root->parent_.lock()) {
            root = p.get();
            continue;
        }
        break;
    }

    root->walkAndExecuteFn(fn);
}

void Component::walkAndExecuteFn(const std::function<void (Component &)>& fn)
{
    fn(*this);
    for(auto& ch : children_) {
        ch->walkAndExecuteFn(fn);
    }
}

void Component::addDeploymentTasks(Component::tasks_t& tasks)
{
    setState(State::RUNNING);
    for(auto& child : children_) {
        child->addDeploymentTasks(tasks);
    }
}

void Component::addRemovementTasks(Component::tasks_t &tasks)
{
    setState(State::RUNNING);
    for(auto& child : children_) {
        child->addRemovementTasks(tasks);
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
    case Kind::JOB:
        return make_shared<JobComponent>(parent, cluster, def);
    case Kind::DEPLOYMENT:
        return make_shared<DeploymentComponent>(parent, cluster, def);
    case Kind::SERVICE:
        return make_shared<ServiceComponent>(parent, cluster, def);
    case Kind::CONFIGMAP:
        return make_shared<ConfigMapComponent>(parent, cluster, def);
    case Kind::SECRET:
        return make_shared<SecretComponent>(parent, cluster, def);
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
        auto child = populate(childDef, cluster, component);
                //createComponent(childDef, component, cluster);
        component->children_.push_back(child);
        //populate(childDef, cluster, child);
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

bool Component::hasKindAsChild(Kind kind) const
{
    for(const auto& child : children_) {
        if (child->getKind() == kind) {
            return true;
        }
    }

    return false;
}

void Component::initChildren()
{
    for(auto& child: children_) {
        child->init();
    }
}

Component::ptr_t Component::addChild(const string &name, Kind kind, const labels_t &labels,
                                     const conf_t& args)
{
    ComponentDataDef def;
    def.labels = labels;
    def.name = name;
    def.kind = toString(kind);
    def.args = args;
    assert(cluster_);
    auto component = createComponent(def, shared_from_this(), *cluster_);
    component->init();
    children_.push_back(component);
    return component;
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
    for(auto p = this; p != nullptr; p = p->parentPtr()) {
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

void Component::Task::setState(Component::Task::TaskState state, bool scheduleRunTasks)
{
    LOG_TRACE << component().logName() << " Task " << name() << " change state from "
              << toString(state_) << " to " << toString(state);

    const bool changed = state_ != state;
    state_ = state;

    if (changed && scheduleRunTasks) {
        component().scheduleRunTasks();
    }
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
                    setState(TaskState::DEPENDENCY_FAILED, false);
                    return true;
                }
            }
        }

        if (!blocked) {
            setState(TaskState::READY, false);
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

const JsonFieldMapping *jsonFieldMappings()
{
    static const JsonFieldMapping mappings = {
        {"namespace_", "namespace"},
        {"template_", "template"},
        {"operator_", "operator"},
        {"continue_", "continue"}
    };

    return &mappings;
}

string fileToJson(const string &pathToFile)
{
    if (!filesystem::is_regular_file(pathToFile)) {
        LOG_ERROR << "Not a file: " << pathToFile;
        throw runtime_error("Not a file: "s + pathToFile);
    }

    string json;
    const filesystem::path path{pathToFile};
    const auto ext = path.extension();
    if (ext == ".yaml") {
        // https://www.commandlinefu.com/commands/view/12218/convert-yaml-to-json
        const auto expr = R"(import sys, yaml, json; json.dump(yaml.load(open(")"s
                + pathToFile
                + R"(","r").read()), sys.stdout, indent=4))"s;
        auto args = boost::process::args({"-c"s, expr});
        boost::process::ipstream pipe_stream;
        boost::process::child process(boost::process::search_path("python"),
                                      args,
                                      boost::process::std_out > pipe_stream);
        string line;
        while (pipe_stream && std::getline(pipe_stream, line)) {
            json += line;
        }

        error_code ec;
        process.wait(ec);
        if (ec) {
            LOG_ERROR << "Failed to convert yaml from " << pathToFile << ": " << ec.message();
            throw runtime_error("Failed to convert yaml: "s + pathToFile);
        }

    } else if (ext == ".json") {
        json = slurp(pathToFile);
    } else {
        LOG_ERROR << "File extension must be yaml or json: " << pathToFile;
        throw runtime_error("Unknown extension "s + pathToFile);
    }

    return json;
}

} // ns
