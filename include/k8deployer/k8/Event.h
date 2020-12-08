#pragma once

#include <string>
#include <vector>
#include <map>

#include <boost/fusion/adapted.hpp>

namespace k8deployer::k8api {

struct Event {
    struct Metadadata {
        using fields_t = std::vector<std::map<std::string, std::string>>;
        std::string name;
        std::string _namespace;
        std::string selfLink;
        std::string uid;
        std::string resourceVersion;
        std::string creationTimestamp;
        std::string deletionTimestamp;
        //fields_t managedFields;
    };

    struct InvolvedObject {
        using finalizers_t = std::vector<std::string>;
        std::string apiVersion;
        std::string fieldPath;
        std::string kind;
        std::string name;
        std::string _namespace;
        std::string resourceVersion;
        std::string uid;
        finalizers_t finalizers;
        //owner_references_t ownerReferences;
        std::string selfLink;
    };

    std::string action;
    std::string apiVersion;
    size_t count = 0;
    //MicroTime eventTime;
    std::string firstTimestamp;
    std::string lastTimestamp;
    std::string kind;
    std::string message;
    std::string reason;
    std::string type;
    std::string reportingComponent;
    std::string reportingInstance;
    Metadadata metadata;
    InvolvedObject involvedObject;
};

struct EventStream {
    std::string type;
    Event object;
};

} // ns

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Event::Metadadata,
                          (std::string, name)
                          (std::string, _namespace)
                          (std::string, selfLink)
                          (std::string, uid)
                          (std::string, resourceVersion)
                          (std::string, creationTimestamp)
                          (std::string, deletionTimestamp)
                          //(k8deployer::k8api::Event::Metadadata::fields_t, managedFields)
                          );

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Event::InvolvedObject,
                          (std::string, apiVersion)
                          (std::string, fieldPath)
                          (std::string, kind)
                          (std::string, name)
                          (std::string, _namespace)
                          (std::string, resourceVersion)
                          (std::string, uid)
                          (k8deployer::k8api::Event::InvolvedObject::finalizers_t, finalizers)
                          //owner_references_t ownerReferences)
                          (std::string, selfLink)
                          );

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::Event,
                          (std::string, action)
                          (std::string, apiVersion)
                          (size_t, count)
                          //MicroTime eventTime)
                          (std::string, firstTimestamp)
                          (std::string, lastTimestamp)
                          (std::string, kind)
                          (std::string, message)
                          (std::string, reason)
                          (std::string, type)
                          (std::string, reportingComponent)
                          (std::string, reportingInstance)
                          (k8deployer::k8api::Event::Metadadata, metadata)
                          (k8deployer::k8api::Event::InvolvedObject, involvedObject)
                          );

BOOST_FUSION_ADAPT_STRUCT(k8deployer::k8api::EventStream,
                          (std::string, type)
                          (k8deployer::k8api::Event, object)
                          );
