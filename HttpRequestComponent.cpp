
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
}

void HttpRequestComponent::addDeploymentTasks(Component::tasks_t &tasks)
{
  auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event */*event*/) {

      // Execution?
      if (task.state() == Task::TaskState::READY) {
          task.setState(Task::TaskState::EXECUTING);

          try {
              sendRequest();
              task.setState(Task::TaskState::DONE);
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

void HttpRequestComponent::sendRequest()
{
    // Create a new rest client without any TLS context,
    auto cli = RestClient::Create();

    cli->ProcessWithPromise([this](restc_cpp::Context& ctx) {
        RequestBuilder builder{ctx};
        builder.Req(url_, rct_);

        if (!user_.empty()) {
            builder.BasicAuthentication(user_, passwd_);
        }

        if (!json_.empty()) {
            builder.Data(json_);
        }

        LOG_DEBUG << "Sending request to: " << url_;

        try {
            auto reply = builder.Execute();
            const auto data = reply->GetBodyAsString(); // Just read the data
            LOG_TRACE << logName() << "Received: " << data;
        } catch (const exception& ex) {
            LOG_WARN << logName() << "Request failed: " << ex.what();
            throw;
        }

    }).get();
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
