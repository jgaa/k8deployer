#pragma once

#include <functional>
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
#include "k8deployer/k8/k8api.h"
#include "k8deployer/k8/Event.h"

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

    /*! A task
     *
     * The root component owns all tasks, and is responsible to iterate over
     * them and execute those that are in READY state, or ABORT all
     * if the overall work-flow is failing.
     *
     * The workflow is that each task is scheduled for execution
     * when it is ready.
     *
     * It's executed by calling it's function. the function is
     * responsible for changing the tasks state until it it DONE
     * or FAILED.
     */
    class Task {
    public:
        enum class TaskState {
            PRE,
            BLOCKED,
            READY,
            EXECUTING,
            WAITING, // Waiting for events to update it's status
            DONE,
            ABORTED,
            FAILED
        };

        // Used both for execution and monitoring
        using fn_t = std::function<void (Task&, const k8api::Event *)>;

        using ptr_t = std::shared_ptr<Task>;
        using wptr_t = std::weak_ptr<Task>;

        Task(Component& component, std::string name, fn_t fn, TaskState initial = TaskState::PRE)
            : component_{component}, name_{std::move(name)}, fn_{std::move(fn)}
            , state_{initial}
        {}

        TaskState state() const noexcept {
            return state_;
        }

        void setState(TaskState state);

        /*! Update the state depending on current state and dependicies.
         *
         * \return true if the state was changed
         */
        bool evaluate();
\
        /*! All tasks in EXECUTING or WAITING state get's the events
         *
         * \return true if the state was changed
         */
        bool onEvent(const k8api::Event& event);

        const std::string& name() const noexcept {
            return name_;
        }

    private:
        // All dependencies must be DONE before the task goes in READY state
        std::deque<wptr_t> dependencies_;

        Component& component_;
        const std::string name_;
        fn_t fn_; // What this task has to do
        TaskState state_ = TaskState::PRE;
    };

    using tasks_t = std::deque<Task::ptr_t>;

    // Since Component is initialized and populated by the json serializer, we can't
    // use overloading to deal with the different kind of components.
    // So, we use WorkFlow to contain the logic specific for these.
    class WorkFlow {
        public:
        virtual ~WorkFlow();

        virtual void init() = 0;
        virtual void prepare() = 0;
        virtual void execute() = 0;
    };

    WorkFlow& workflow() noexcept {
        assert(workflow_);
        return *workflow_;
    }

    Component() = default;
    Component(Cluster& cluster)
        : cluster_{&cluster}
    {}
    Component(Component& parent, Cluster& cluster)
        : parent_{&parent}, cluster_{&cluster}
    {}

    virtual ~Component() = default;

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
    k8api::Service service;

    // Related components owned by this; like volumes or configmaps or service.
    childrens_t children;

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

protected:
    void init_();
    void prepare_();
    void deployComponent();
    void prepareComponent();
    void validate();
    void buildDependencies();
    void buildDeploymentDependencies();
    bool hasKindAsChild(Kind kind) const;
    void prepareDeploy();
    void prepareService();
    void initChildren();
    tasks_t buildDeployDeploymentTasks();
    tasks_t buildDeployServiceTasks();

    // Build the DeployTasks list for this component
    tasks_t buildDeployTasks();

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
    std::unique_ptr<WorkFlow> workflow_;
};

class RootComponent: public Component {
public:
    RootComponent(Cluster& cluster)
        : Component(cluster)
    {}

    void init();
    void prepare();
    void execute();

private:
    void buildTasks();

    tasks_t tasks_;
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

