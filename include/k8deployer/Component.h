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

const restc_cpp::JsonFieldMapping *jsonFieldMappings();

using conf_t = std::map<std::string, std::string>;
using labels_t = std::map<std::string, std::string>;

struct ComponentData {
    virtual ~ComponentData() = default;

    std::string name;
    labels_t labels;
    conf_t defaultArgs; // Added to args and childrens args, unless overridden
    conf_t args;

    // Can be populated by configuration, but normally we will do it
    k8api::Deployment deployment;
    k8api::Service service;
};

struct ComponentDataDef : public ComponentData {
    // Related components owned by this; like volumes or configmaps or service.

    std::string kind;
    std::string parentRelation;

    using childrens_t = std::deque<ComponentDataDef>;
    childrens_t children;
};

/*! Tree of components to work with.
 *
 */
class Component : protected ComponentData,
        public std::enable_shared_from_this<Component> {
public:
    using ptr_t = std::shared_ptr<Component>;
    using childrens_t = std::deque<Component::ptr_t>;

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
            FAILED,
            DEPENDENCY_FAILED
        };

        /*! State machine, used both for execution and monitoring
         *
         * It is called for execution when the Task is in READY state,
         * and again for each k8 event received while it's in EXECUTING
         * or WAITING state.
         *
         * \param task The calling Task
         * \param event nullptr if the task is in READY state, in which case
         *      the task is executed and the state updated.
         *      Pointer to an event if it is called in EXECUTING or WAITING
         *      state.
         */
        using fn_t = std::function<void (Task& task, const k8api::Event *event)>;

        using ptr_t = std::shared_ptr<Task>;
        using wptr_t = std::weak_ptr<Task>;

        Task(Component& component, std::string name, fn_t fn, TaskState initial = TaskState::PRE)
            : component_{component}, name_{std::move(name)}, fn_{std::move(fn)}
            , state_{initial}
        {}

        TaskState state() const noexcept {
            return state_;
        }

        bool isMonitoring() const noexcept {
            return state_ == TaskState::EXECUTING || state_ == TaskState::WAITING;
        }

        bool isDone() const noexcept {
            return state_ >= TaskState::DONE;
        }

        void setState(TaskState state);

        /*! Update the state depending on current state and dependicies.
         *
         * If in BLOCKED state, the Task will change it's state to READY when
         * it determines that it is unblocked.
         *
         * \return true if the state was changed
         */
        bool evaluate();

        void execute() {
            assert(state_ == TaskState::READY);
            fn_(*this, {});
        }
\
        /*! All tasks in EXECUTING or WAITING state get's the events
         *
         * \return true if the state was changed
         */
        bool onEvent(const k8api::Event& event) {
            const auto startState = state_;
            fn_(*this, &event);
            return state_ != startState;
        }

        const std::string& name() const noexcept {
            return name_;
        }

        Component& component() {
            return component_;
        }

        static const std::string& toString(const TaskState& state);

    private:
        // All dependencies must be DONE before the task goes in READY state
        std::deque<wptr_t> dependencies_;

        Component& component_;
        const std::string name_;
        fn_t fn_; // What this task has to do
        TaskState state_ = TaskState::PRE;
    };

    using tasks_t = std::deque<Task::ptr_t>;


    Component(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data)
        : ComponentData(data), parent_{parent}, cluster_{&cluster}
    {}

    virtual ~Component() = default;

    // When to execute a child, relative to the parent
    enum class ParentRelation {
        INDEPENDENT,
        BEFORE,
        AFTER
    };

    Kind getKind() const noexcept {
        return kind_;
    };

    // Prefix for logging
    std::string logName() const noexcept;

    std::optional<bool> getBoolArg(const std::string& name) const;
    std::optional<std::string> getArg(const std::string& name) const;
    std::string getArg(const std::string& name, const std::string& defaultVal) const;
    int getIntArg(const std::string& name, int defaultVal) const;
    size_t getSizetArg(const std::string &name, size_t defaultVal) const;

    Cluster& cluster() noexcept {
        assert(cluster_);
        return *cluster_;
    }

    static ptr_t populateTree(const ComponentDataDef &def, Cluster& cluster);

    // Called on the root component
    // Let clusters prepare themselves to de deployed in parallell
    virtual std::future<void> prepareDeploy();

    // Called on the root component
    // Let clusters deploy themselfs in parallell
    std::future<void> deploy();

    // Called on the root component
    void onEvent(const std::shared_ptr<k8api::Event>& event);

    labels_t::value_type getSelector();

protected:
    void processEvent(const k8api::Event& event);

    // Recursively add tasks to the task list
    virtual void addTasks(tasks_t& tasks);

    static Component::ptr_t createComponent(const ComponentDataDef &def,
                                     const Component::ptr_t& parent,
                                     Cluster& cluster);

    static Component::ptr_t populate(const ComponentDataDef &def,
                                     Cluster& cluster,
                                     const Component::ptr_t& parent);

    std::future<void> dummyReturnFuture() const;
    void init();
    void prepare();
    void validate();
    bool hasKindAsChild(Kind kind) const;
    void initChildren();

    // Build the DeployTasks list for this component
    tasks_t buildDeployTasks();

    ptr_t addChild(const std::string& name, Kind kind, const labels_t& labels);
    conf_t mergeArgs() const;

    // Get a path to root, where the current node is first in the list
    std::vector<const Component *> getPathToRoot() const;
    const Component *parentPtr() const;
    void runTasks();
    bool allTasksAreDone() const noexcept {
        return state_ >= State::DONE;
    }

    void setState(State state) {
        state_ = state;
    }

    bool isDone() const noexcept {
        return state_ >= State::DONE;
    }

    State state_{State::PRE}; // From our logic
    std::string k8state_; // From the event-loop
    std::weak_ptr<Component> parent_;
    Cluster *cluster_ = {};
    ParentRelation parentRelation_ = ParentRelation::AFTER;
    Kind kind_ = Kind::APP;
    conf_t effectiveArgs_;
    childrens_t children_;
    std::unique_ptr<tasks_t> tasks_;
    std::unique_ptr<std::promise<void>> executionPromise_;
};

} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::ComponentDataDef,
                          (std::string, name)
                          (k8deployer::labels_t, labels)
                          (k8deployer::conf_t, defaultArgs)
                          (k8deployer::conf_t, args)
                          (k8deployer::k8api::Deployment, deployment)
                          (k8deployer::k8api::Service, service)
                          (std::string, kind)
                          (std::string, parentRelation)
                          (k8deployer::ComponentDataDef::childrens_t, children)
                          );

