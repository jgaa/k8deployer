#pragma once

#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/Engine.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Component.h"
#include "k8deployer/logging.h"

namespace k8deployer {

template <typename T, typename TvalidateFn>
void sendProbe(Component& component, const std::string& url,
               std::function<void(const std::optional<T>& object, Component::K8ObjectState state)> onDone,
               TvalidateFn && validate)
{
    Engine::client().Process([url, &component,
                             onDone=std::move(onDone),
                             validate=std::move(validate)](auto& ctx) {

        LOG_TRACE << component.logName() << "Probing";

        try {
            T data;
            auto reply = restc_cpp::RequestBuilder{ctx}.Get(url)
                    .Execute();

            restc_cpp::SerializeFromJson(data, *reply);
            const auto done = validate(data);

            LOG_TRACE << component.logName()
                  << "Probing gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase
                  << ", done = " << (done ? "yes": "no");

            onDone(data, done ? Component::K8ObjectState::DONE : Component::K8ObjectState::INIT);
            return;
        } catch(const restc_cpp::RequestFailedWithErrorException& err) {

            if (err.http_response.status_code == 404) {
                LOG_TRACE << component.logName()
                     << "Probing failed: " << err.http_response.status_code
                     << ' ' << err.http_response.reason_phrase
                     << ": " << err.what();
            } else {
                LOG_WARN << component.logName()
                     << "Probing failed: " << err.http_response.status_code
                     << ' ' << err.http_response.reason_phrase
                     << ": " << err.what();
            }

            onDone({}, err.http_response.status_code == 404 ?
                     Component::K8ObjectState::DONT_EXIST :
                     Component::K8ObjectState::FAILED);

        } catch(const std::exception& ex) {
            LOG_WARN << component.logName()
                     << "Probing failed: " << ex.what();
            onDone({}, Component::K8ObjectState::FAILED);
        }
    });
}

} // ns
