#pragma once

#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <cassert>
#include <sstream>

#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/RequestBuilder.h"

#include "k8deployer/Config.h"
#include "k8deployer/k8/k8api.h"
#include "k8deployer/Engine.h"
#include "k8deployer/logging.h"
#include "k8deployer/DataDef.h"

namespace k8deployer {

class Cluster;
class Component;

enum class Kind {
    APP, // A placeholder that owns other components
    JOB,
    DEPLOYMENT,
    STATEFULSET,
    SERVICE,
    CONFIGMAP,
    SECRET,
    PERSISTENTVOLUME,
    INGRESS,
    NAMESPACE,
    DAEMONSET,
    ROLE,
    CLUSTERROLE,
    ROLEBINDING,
    CLUSTERROLEBINDING,
    SERVICEACCOUNT
};

std::string slurp (const std::string& path);

Kind toKind(const std::string& kind);

std::string toString(const Kind& kind);

const restc_cpp::JsonFieldMapping *jsonFieldMappings();
std::string expandVariables(const std::string& json, const variables_t& vars);

/*! Reads the contents from a .json or .yaml file and returns the content as json. */
std::string fileToJson(const std::string& pathToFile, bool assumeYaml = false);

/*! Reads the contents from a .json or .yaml file and serialize it to obj */
template <typename T>
void fileToObject(T& obj, const std::string& pathToFile, const variables_t& vars) {
    const auto json = fileToJson(pathToFile);
    const auto expandedJson = expandVariables(json, vars);
    std::istringstream ifs{expandedJson};
    restc_cpp::serialize_properties_t properties;
    properties.ignore_unknown_properties = false;
    properties.name_mapping = jsonFieldMappings();

    restc_cpp::SerializeFromJson(obj, ifs);
}

template <typename T>
std::string toJson(const T& obj) {
    std::ostringstream out;
    restc_cpp::serialize_properties_t properties;
    properties.name_mapping = jsonFieldMappings();
    restc_cpp::SerializeToJson(obj, out, properties);
    return out.str();
}

std::string Base64Encode(const std::string &in);

std::future<void> dummyReturnFuture();


/*! Tree of components to work with.
 *
 */
class Component : public ComponentData,
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

    enum class Mode {
        CREATE,
        REMOVE
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
    class Task : public std::enable_shared_from_this<Task> {
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

        Task(Component& component, std::string name, fn_t fn,
             TaskState initial = TaskState::PRE, Mode mode = Mode::CREATE)
            : component_{component}, name_{std::move(name)}, fn_{std::move(fn)}
            , state_{initial}, mode_{mode}
        {}

        TaskState state() const noexcept {
            return state_;
        }

        Mode mode() const noexcept {
            return mode_;
        }

        bool isMonitoring() const noexcept {
            return state_ == TaskState::EXECUTING || state_ == TaskState::WAITING;
        }

        bool isDone() const noexcept {
            return state_ >= TaskState::DONE;
        }

        void setState(TaskState state, bool scheduleRunTasks = true);

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

        // Schedule a new poll, unless one is already scheduled
        void schedulePoll();
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

        void addDependency(const wptr_t& task) {
            dependencies_.push_back(task);
        }

        void addAllDependencies(std::set<Task *>& tasks);

        bool startProbeAfterApply = false;
        bool dontFailIfAlreadyExists = false;

    private:
        // All dependencies must be DONE before the task goes in READY state
        std::deque<wptr_t> dependencies_;

        Component& component_;
        const std::string name_;
        fn_t fn_; // What this task has to do
        TaskState state_ = TaskState::PRE;
        std::unique_ptr<boost::asio::deadline_timer> pollTimer_;
        const Mode mode_ = Mode::CREATE;
    };

    using tasks_t = std::deque<Task::ptr_t>;


    Component(const Component::ptr_t& parent, Cluster& cluster, const ComponentData& data);

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
    k8api::string_list_t getArgAsStringList(const std::string& values, const std::string& defaultVal) const;
    k8api::env_vars_t getArgAsEnvList(const std::string& values, const std::string& defaultVal) const;
    static k8api::string_list_t getArgAsStringList(const std::string& values);
    static k8api::env_vars_t getArgAsEnvList(const std::string& values);
    static k8api::key_values_t getArgAsKv(const std::string& values);

    std::string getArg(const std::string& name, const std::string& defaultVal) const;
    int getIntArg(const std::string& name, int defaultVal) const;
    size_t getSizetArg(const std::string &name, size_t defaultVal) const;

    Cluster& cluster() noexcept {
        assert(cluster_);
        return *cluster_;
    }

    restc_cpp::RestClient& client() {
        return cluster().client();
    }

    static ptr_t populateTree(const ComponentDataDef &def, Cluster& cluster);

    // Called on the root component
    virtual void prepareDeploy();

    // Called on the root component
    // Let clusters deploy themselfs in parallell
    std::future<void> deploy();

    // Called on the root component
    // Let clusters delete themselfs in parallell
    std::future<void> remove();

    // Called on the root component
    void onEvent(const std::shared_ptr<k8api::Event>& event);

    labels_t::value_type getSelector();

    ParentRelation parentRelation() const noexcept {
        return parentRelation_;
    }

    // Called on any component when a task-status change out of order.
    void scheduleRunTasks();

    childrens_t& getChildren() {
        return children_;
    }

    enum class K8ObjectState {
        FAILED,
        DONT_EXIST,
        INIT,
        READY,
        DONE
    };

    /*! Probe to check the state of the correcsponding kubernetes object
     *
     *  \param probe Callback when the probe is done. If empty, the method will just
     *      return weather the object can be probed or not.
     *
     *  \return true if the object can be probed (if it can have a corresponding k8 object)
     */
    virtual bool probe(std::function<void(K8ObjectState state)>) {
        return false;
    }

    void schedule(std::function<void ()> fn);

    bool isBlockedOnDependency() const;
    void scanDependencies();

    Component& getRoot();

    // Update state, based on the childrens states
    void evaluate();

    virtual std::string getNamespace() const;

    void startElapsedTimer();

    ptr_t addChild(const std::string& name, Kind kind, const labels_t& labels = {},
                   const conf_t& args = {}, const std::string& parentRelation = {});

    bool isRoot() const {
        return parent_.expired();
    }

protected:
    virtual std::string getCreationUrl() const {
        assert(false); // Implement!
    };

    virtual std::string getAccessUrl() const {
        return getCreationUrl() + "/" + name;
    };

    void addDependenciesRecursively(std::set<Component *>& contains);
    void processEvent(const k8api::Event& event);

    // Recursively add tasks to the task list
    virtual void addDeploymentTasks(tasks_t& tasks);
    virtual void addRemovementTasks(tasks_t& tasks);

    static Component::ptr_t createComponent(const ComponentDataDef &def,
                                     const Component::ptr_t& parent,
                                     Cluster& cluster);

    static Component::ptr_t populate(const ComponentDataDef &def,
                                     Cluster& cluster,
                                     const Component::ptr_t& parent);

    void init();
    void prepare();
    void validate();
    bool hasKindAsChild(Kind kind) const;
    Component * getFirstKindAmongChildren(Kind kind);
    void initChildren();
    std::future<void> execute(std::function<void(tasks_t&)> fn);

    // Build the DeployTasks list for this component
    tasks_t buildDeployTasks();

    conf_t mergeArgs() const;

    // Get a path to root, where the current node is first in the list
    std::vector<const Component *> getPathToRoot() const;
    const Component *parentPtr() const;
    void runTasks();
    bool allTasksAreDone() const noexcept {
        return state_ >= State::DONE;
    }

    void setState(State state);

    bool isDone() const noexcept {
        return state_ >= State::DONE;
    }

    // Top town invocation of `fn` on eachg component, starting with root
    void forAllComponents(const std::function<void (Component&)>& fn);
    void walkAndExecuteFn(const std::function<void (Component&)>& fn);

    virtual void buildDependencies() {}

    template <typename T>
    void sendApply(const T& data, const std::string& url, std::weak_ptr<Task> task,
                   const restc_cpp::Request::Type requestType = restc_cpp::Request::Type::POST)
    {
        // Create the json payload here for two reasons:
        //  1) kubernetes don't seem to like chunked bodies for patch payloads
        //  2) We have no guarantee regarding the lifetime of the data object.
        auto json = toJson(data);

        cluster_->client().Process([this, url, task, json=std::move(json), requestType](auto& ctx) {
            std::string taskName = "***";
            if (auto t = task.lock()) {
                taskName = t->name();
            }
            LOG_DEBUG << logName() << "Applying task " << taskName << " to " << url;
            LOG_TRACE << logName() << "Applying payload for task " << taskName << ": " << json;
            std::string contentType = "application/json; charset=utf-8";
            if (requestType == restc_cpp::Request::Type::PATCH) {
                contentType = "application/merge-patch+json; charset=utf-8";
            }

            try {
                auto reply = restc_cpp::RequestBuilder{ctx}.Req(url, requestType)
                   .Header("Content-Type", contentType)
                   .Data(json)
                   .Execute();

                LOG_DEBUG << logName()
                      << "Applying task " << taskName << " gave response: "
                      << reply->GetResponseCode() << ' '
                      << reply->GetHttpResponse().reason_phrase;

                if (auto t = task.lock()) {
                    if (t->startProbeAfterApply) {
                        t->setState(Task::TaskState::WAITING);
                        t->schedulePoll();
                    } else {
                        // Assume that tasks that don't need polling are OK after create.
                        t->setState(Task::TaskState::DONE);
                    }
                }

                return;
            } catch(const restc_cpp::RequestFailedWithErrorException& err) {
                if (err.http_response.status_code == 404) {
                    if (auto t = task.lock()) {
                        if (t->mode() == Mode::REMOVE) {
                            LOG_DEBUG << logName()
                                      << "Applying REMOVE task " << taskName << " to already deleted resource. Probably ok: "
                                      << err.http_response.status_code << ' '
                                      << err.http_response.reason_phrase;
                            t->setState(Task::TaskState::DONE);
                            return;
                        }
                    }
                }

                if (err.http_response.status_code == 409) {
                    if (auto t = task.lock()) {
                        if (t->mode() == Mode::CREATE && t->dontFailIfAlreadyExists) {
                            LOG_DEBUG << logName()
                                      << "Applying task " << taskName << " to existing resource. Probably ok: "
                                      << err.http_response.status_code << ' '
                                      << err.http_response.reason_phrase;
                            t->setState(Task::TaskState::DONE);
                            return;
                        }
                    }
                }

                LOG_WARN << logName()
                         << "Apply task " << taskName << ": Request failed: " << err.http_response.status_code
                         << ' ' << err.http_response.reason_phrase
                         << ": " << err.what();

            } catch(const std::exception& ex) {
                LOG_WARN << logName()
                         << "Apply task " << taskName << ": Request failed: " << ex.what();
            }

            if (auto taskInstance = task.lock()) {
                taskInstance->setState(Task::TaskState::FAILED);
            }
            setState(State::FAILED);

        });
    }

    void sendDelete(const std::string& url, std::weak_ptr<Component::Task> task,
                    bool ignoreErrors = false,
                    const std::initializer_list<std::pair<std::string, std::string>>& args = {});

    void calculateElapsed();
    void addDependencyToNamespace();

    State state_{State::PRE}; // From our logic
    std::string k8state_; // From the event-loop
    std::weak_ptr<Component> parent_;
    Cluster *cluster_ = {};
    ParentRelation parentRelation_ = ParentRelation::INDEPENDENT;
    Kind kind_ = Kind::APP;
    conf_t effectiveArgs_;
    childrens_t children_;
    std::unique_ptr<tasks_t> tasks_;
    std::unique_ptr<std::promise<void>> executionPromise_;
    bool reverseDependencies_ = false;
    std::vector<std::weak_ptr<Component>> dependsOn_;
    Mode mode_ = Mode::CREATE;
    std::optional<std::chrono::steady_clock::time_point> startTime;
    std::optional<double> elapsed = {};
};

} // ns

