
#include <regex>

#include "include/k8deployer/HttpRequestComponent.h"

#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/logging.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Engine.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

HttpRequestComponent::HttpRequestComponent(const Component::ptr_t &parent, Cluster &cluster, const ComponentData &data)
    : Component(parent, cluster, data)
{
  kind_ = Kind::HTTP_REQUEST;
}

void HttpRequestComponent::prepareDeploy()
{
   if (auto target = getArg("target")) {
       static const regex targetPattern{R"((GET|POST|PATCH|PUT|DELETE|OPTIONS|HEAD) +(.+))"};
       smatch tokens;
       if (regex_match(*target, tokens, targetPattern)) {
          const auto typeName = tokens.str(1);
          rct_ = toType(typeName);
          url_ = tokens.str(2);
       } else {
           LOG_ERROR << logName() << "Invalid target specified: " << *target;
           LOG_ERROR << logName() << "Target must be: GET|POST|PATCH|PUT|DELETE|OPTIONS|HEAD <space> url";
           throw runtime_error("Invalid target");
       }
   } else {
       LOG_ERROR << logName() << "No target specified";
       throw runtime_error("No target");
   }

   json_ = getArg("json", "");
   message_ = getArg("log.message", "");
   if (auto auth = getArg("auth")) {
       auto kv = getArgAsKv(*auth);
       if (auto user = kv.find("user") ; user != kv.end()) {
            user_ = user->second;

           if (auto passwd = kv.find("passwd") ; passwd != kv.end()) {
              passwd_ = passwd->second;
           } else {
              LOG_WARN << logName() << "Auth: 'user' is specified, but not `passwd`. That is unusual.";
           }
       }
   }

   retries_ = getIntArg("retry.count", retries_);
   retryDelaySeconds_ = getIntArg("retry.delay.seconds", retryDelaySeconds_);
}

void HttpRequestComponent::addDeploymentTasks(Component::tasks_t &tasks)
{
  auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event */*event*/) {

      // Execution?
      if (task.state() == Task::TaskState::READY) {
          task.setState(Task::TaskState::EXECUTING);

          try {
              auto wt = task.weak_from_this();
              if (!message_.empty()) {
                  LOG_INFO << logName() << message_;
              }
              sendRequest(wt);
          } catch (const exception& ex) {
              LOG_ERROR << logName() << "Failed to call url " << url_
                        << ": " << ex.what();
              task.setState(Task::TaskState::FAILED);
          }
      }

      task.evaluate();
  });

  tasks.push_back(task);
  Component::addDeploymentTasks(tasks);
}

void HttpRequestComponent::sendRequest(weak_ptr<Task>& wtask)
{
    // Create a new rest client without any TLS context,
    client_ = RestClient::Create(cluster_->client().GetIoService());

    client_->Process([this, wtask](restc_cpp::Context& ctx) {
        while(cluster_->isExecuting()) {
          auto task = wtask.lock();
          if (!task) {
              LOG_ERROR << logName() << "Task vanished";
              return;
          }

          if (task->state() != Task::TaskState::EXECUTING) {
              return;
          }

          RequestBuilder builder{ctx};
          builder.Req(url_, rct_);

          if (!user_.empty()) {
              builder.BasicAuthentication(user_, passwd_);
          }

          if (!json_.empty()) {
              builder.Data(json_);
          }

          LOG_DEBUG << logName() << "Sending request to: " << url_;
          LOG_DEBUG << logName() << "Payload: " << json_;

          // Release the reference to the task before we enter async operations
          task.reset();

          try {
              auto reply = builder.Execute();
              const auto data = reply->GetBodyAsString(); // Just read the data
              LOG_TRACE << logName() << "Received: " << data;

              if (task = wtask.lock() ; task) {
                 task->setState(Task::TaskState::DONE);
              }
          } catch (const exception& ex) {
              LOG_WARN << logName() << "Request failed: " << ex.what();
              if (task = wtask.lock(); task) {
                 if (currentCnt_ >= retries_) {
                    LOG_ERROR << logName() << "Request failed. No retries left: " << ex.what();
                    task->setState(Task::TaskState::FAILED);
                 } else {
                    ++currentCnt_;
                    LOG_INFO << logName() << "Retrying request in " << retryDelaySeconds_ << " seconds";
                    task->setState(Task::TaskState::WAITING);
                    ctx.Sleep(std::chrono::seconds(retryDelaySeconds_));
                    task->setState(Task::TaskState::EXECUTING);
                 }
              }
           }
        } // retry loop
    });
}

Request::Type HttpRequestComponent::toType(const string &name)
{
    static const std::map<std::string, restc_cpp::Request::Type> types = {
        {"GET", restc_cpp::Request::Type::GET},
        {"POST", restc_cpp::Request::Type::POST},
        {"PUT", restc_cpp::Request::Type::POST},
        {"DELETE", restc_cpp::Request::Type::DELETE},
        {"OPTIONS", restc_cpp::Request::Type::OPTIONS},
        {"HEAD", restc_cpp::Request::Type::HEAD},
        {"PATCH", restc_cpp::Request::Type::PATCH}
    };

    return types.at(name);
}



} // ns
