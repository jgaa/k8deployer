#pragma once

#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/RequestBuilder.h"
#include "k8deployer/Engine.h"
#include "k8deployer/Cluster.h"
#include "k8deployer/Component.h"
#include "k8deployer/logging.h"

namespace k8deployer {

template <typename T>
void sendProbe(Component& component, const std::string& url,
               std::function<void(const std::optional<T>& object, Component::K8ObjectState state)> onDone)
{
    Engine::client().Process([url, &component, onDone=std::move(onDone)](auto& ctx) {

        LOG_TRACE << component.logName() << "Probing";

        try {
            T data;
            auto reply = restc_cpp::RequestBuilder{ctx}.Get(url)
                    .Execute();

            LOG_TRACE << component.logName()
                  << "Probing gave response: "
                  << reply->GetResponseCode() << ' '
                  << reply->GetHttpResponse().reason_phrase;

            restc_cpp::SerializeFromJson(data, *reply);

            bool done = false;
            if constexpr (std::is_same_v<T, k8api::Service>) {
                // If it exists, it's probably OK...
                done = true;
            }

            if constexpr (std::is_same_v<T, k8api::StatefulSet>) {
                LOG_TRACE << component.logName()
                        << "Probing: readyReplicas = " << data.status->readyReplicas
                        << ", replicas = " << data.spec.replicas
                        << ", currentReplicas = " << data.status->currentReplicas;

                done = data.status->readyReplicas == data.spec.replicas;
            }

            if constexpr (std::is_same_v<T, k8api::PersistentVolume>) {
                LOG_TRACE << component.logName() << "Probing: message = " << data.status->message
                          << ", phase = " << data.status->phase
                          << ", reason = " << data.status->reason;
                done = data.status->phase == "Available";
            }

            if constexpr (std::is_same_v<T, k8api::Deployment> || std::is_same_v<T, k8api::Job>) {
                for(const auto& cond : data.status->conditions) {
                    if constexpr (std::is_same_v<T, k8api::Deployment>
                            || std::is_same_v<T, k8api::Job>) {
                        if (cond.type == "Available" && cond.status == "True") {
                            done = true;
                        }
                    } else if constexpr (std::is_same_v<T, k8api::Job>) {
                        if (cond.type == "Complete" && cond.status == "True") {
                            done = true;
                        }
                    } else {
                        assert(false); // Unsupported type
                    }
                 }
             }

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
