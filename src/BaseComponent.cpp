
#include "k8deployer/BaseComponent.h"
#include "k8deployer/DeploymentComponent.h"
#include "k8deployer/Cluster.h"

using namespace std;
using namespace string_literals;
using namespace restc_cpp;

namespace k8deployer {

// Does the general preparations to prepare a Job, Deoplyment or StatefulSet
void BaseComponent::basicPrepareDeploy()
{
    if (auto meta = getMetadata()) {
        if (meta->name.empty()) {
            meta->name = name;
        }

        if (meta->namespace_.empty()) {
            meta->namespace_ = getNamespace();
        }

        auto selector = getSelector();

        // Make sure we have a selector
        meta->labels.emplace(selector);

        // Copy all labels that is not already defined in the meta
        for (const auto& label : labels) {
            meta->labels.emplace(label);
        }

        if (auto podTemplate = getPodTemplate()) {
            if (podTemplate->metadata.name.empty()) {
                podTemplate->metadata.name = name;
            }

            if (podSpecSecurityContext && !podTemplate->spec.securityContext) {
                podTemplate->spec.securityContext = *podSpecSecurityContext;
            }

            if (auto arg = getArg("serviceAccountName")
                    ; arg && podTemplate->spec.serviceAccountName.empty()) {
                podTemplate->spec.serviceAccountName = *arg;
            }

            // Copy all labels from meta that is not already degined in the pod
            for (const auto& label : meta->labels) {
                podTemplate->metadata.labels.emplace(label);
            }

            k8api::Container container;
            container.name = name;
            container.image = getArg("image", name);
            container.args = getArgAsStringList("pod.args", ""s);
            container.env = getArgAsEnvList("pod.env", ""s);
            filterEnvVars(container.env);

            container.command = getArgAsStringList("pod.command", ""s);
            container.imagePullPolicy = getArg("imagePullPolicy", {});

            if (podSecurityContext) {
                container.securityContext = *podSecurityContext;
            }

            if (auto psca = getArgAsStringList("pod.scc.add", ""s); !psca.empty()) {
                if (!container.securityContext) {
                    container.securityContext.emplace();
                }

                if (!container.securityContext->capabilities) {
                    container.securityContext->capabilities.emplace();
                }

                for(const auto& c: psca) {
                    container.securityContext->capabilities->add.push_back(c);
                }
            }

            if (auto ports = parsePorts(getArg("port", ""s)); !ports.empty()) {
                for(const auto& port: ports) {
                    k8api::ContainerPort p;
                    p.containerPort = port.port;
                    p.name = port.getName();
                    p.protocol = port.protocol;
                    container.ports.emplace_back(p);
                }
            }

            container.startupProbe = startupProbe;
            container.livenessProbe = livenessProbe;
            container.readinessProbe = readinessProbe;

            auto resources = [&container]() -> k8api::ResourceRequirements& {
                if (!container.resources) {
                    container.resources.emplace();
                }
                return *container.resources;
            };

            if (!Engine::config().ignoreResourceLimits) {
                // TODO: Add `resources.limits.hugepages-*`
                // https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/

                if (auto v = getArg("pod.limits.memory"); v && !v->empty()) {
                    resources().limits["memory"] = *v;
                } else if (auto v = getArg("pod.memory"); v && !v->empty()) {
                    resources().limits["memory"] = *v;
                }

                if (auto v = getArg("pod.limits.cpu"); v && !v->empty()) {
                    resources().limits["cpu"] = *v;
                } else if (auto v = getArg("pod.cpu"); v && !v->empty()) {
                    resources().limits["cpu"] = *v;
                }

                if (auto v = getArg("pod.requests.memory"); v && !v->empty()) {
                    resources().requests["memory"] = *v;
                } else if (auto v = getArg("pod.memory"); v && !v->empty()) {
                    resources().requests["memory"] = *v;
                }

                if (auto v = getArg("pod.requests.cpu"); v && !v->empty()) {
                    resources().requests["cpu"] = *v;
                } else if (auto v = getArg("pod.cpu"); v && !v->empty()) {
                    resources().requests["cpu"] = *v;
                }
            }

            if (auto dhcred = getArg("imagePullSecrets")) {
                // Use existing secret|
                if (!dhcred->empty()) {
                    k8api::LocalObjectReference lor = {*dhcred};
                    podTemplate->spec.imagePullSecrets.push_back(lor);
                    LOG_DEBUG << logName() << "Using imagePullSecret " << *dhcred;
                }
            }

            if (auto tls = getArg("tls.secret")) {
                // Use existing secret

                k8api::VolumeMount vm;
                vm.name = "tls-secret";
                vm.mountPath = "/certs";
                vm.readOnly = true;
                container.volumeMounts.push_back(vm);

                k8api::Volume v;
                v.name = "tls-secret";
                v.secret.emplace();
                v.secret->secretName = tls.value();
                podTemplate->spec.volumes.push_back(v);
            }

            // TODO: Consider to merge existing pod if it exists
            // (if the user declared it to set some poperties)
            podTemplate->spec.containers.push_back(move(container));
        }

        if (auto ls = getLabelSelector()) {
            ls->matchLabels.emplace(selector);
        }
    }
}

void BaseComponent::addDeploymentTasks(Component::tasks_t& tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event *event) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            buildInitContainers(); // This must be done after all components are initialized
            task.setState(Task::TaskState::EXECUTING);
            doDeploy(task.weak_from_this());
        }

        // Monitoring?
        if (task.isMonitoring() && event) {
            LOG_TRACE << task.component().logName() << " Task " << task.name()
                      << " evaluating event: " << event->message;

            auto key = name + "-";
            if (event->involvedObject.kind == "Pod"
                && event->involvedObject.name.substr(0, key.size()) == key
                && event->metadata.namespace_ == deployment.metadata.namespace_
                && event->metadata.name.substr(0, key.size()) == key
                && event->reason == "Created") {

                // A pod with our name was created.
                // TODO: Use this as a hint and query the status of running pods.
                // In order to continue, all our pods should be in avaiable state

//                LOG_DEBUG << "A pod with my name was created. I am assuming it relates to me";
//                if (++podsStarted_ >= deployment.spec.replicas) {
//                    task.setState(Task::TaskState::DONE);
//                    setState(State::DONE);
//                }
            }
        }

        task.evaluate();
    });

    task->startProbeAfterApply = probe(nullptr);
    tasks.push_back(task);
    Component::addDeploymentTasks(tasks);
}

void BaseComponent::addRemovementTasks(Component::tasks_t &tasks)
{
    auto task = make_shared<Task>(*this, name, [&](Task& task, const k8api::Event */*event*/) {

        // Execution?
        if (task.state() == Task::TaskState::READY) {
            task.setState(Task::TaskState::EXECUTING);
            doRemove(task.weak_from_this());
            task.setState(Task::TaskState::WAITING);
        }

        task.evaluate();
    }, Task::TaskState::READY);

    tasks.push_back(task);
    Component::addRemovementTasks(tasks);
}

void BaseComponent::filterEnvVars(k8api::env_vars_t &vars)
{
    for(const auto& excluded : Engine::config().removeEnvVars) {
search_again:
          if (auto it = find_if(vars.begin(),vars.end(),
                             [&excluded](const auto& e){
              return e.name == excluded;
        }) ; it != vars.end()) {
            LOG_TRACE << logName() << "Removing env-var " << excluded;
            vars.erase(it);
            // We are still at C++17 (no erase_if()),
            // and there may be more of this, so re-iterate.
            goto search_again;
        }
    }
}

void BaseComponent::buildInitContainers()
{
    // Check dependencies
    if (auto podTemplate = getPodTemplate()) {
        for(const auto& name: depends) {
            // Add dependencies for any service with, or assigned to, that name
            forAllComponents([&](Component& c) {
                Component *target = {};
                if (name == c.name) {
                   if (c.getKind() == Kind::SERVICE) {
                       target = &c;
                   } else if (c.getKind() == Kind::DEPLOYMENT
                              || c.getKind() == Kind::STATEFULSET
                              || c.getKind() == Kind::DAEMONSET) {
                       for(auto& cc : c.getChildren()) {
                           if (cc->getKind() == Kind::SERVICE) {
                               target = cc.get();
                               break;
                           }
                       }
                   }
                }

                if (target) {
                    k8api::Container init;
                    init.command = {"sh"s,
                                    "-c"s,
                                    "until nslookup "s + target->name + "; "
                                    + " do echo waiting for "s + target->name
                                    + "; sleep 2; done;"};
                    init.image = "busybox";
                    init.name = "init-"s + c.name + "-" + target->name;

                    if (Engine::config().skipDependencyInitContainers) {
                        LOG_INFO << logName()
                                 << "Skipping dependency to " << target->logName()
                                 << "initContainer " << init.name;
                    } else {
                      LOG_DEBUG << logName()
                                << "Adding dependency to " << target->logName()
                                << "initContainer " << init.name;

                      podTemplate->spec.initContainers.push_back(move(init));
                    }
                }
            });
        }

        // chown/chmod volumes
        for (auto storageDef : storage) {
            if (!storageDef.chownUser.empty()
                    || !storageDef.chownGroup.empty()
                    || !storageDef.chmodMode.empty()) {
                k8api::Container init;

                string cmd;
                if (!storageDef.chownUser.empty()) {
                    cmd += "chown -R "s
                        + storageDef.chownUser
                        + " " + storageDef.volume.mountPath + " ; ";
                }

                if (!storageDef.chownGroup.empty()) {
                    cmd += "chgrp -R "s
                        + storageDef.chownGroup
                        + " " + storageDef.volume.mountPath + " ; ";
                }

                if (!storageDef.chmodMode.empty()) {
                    cmd += "chmod -R "s
                        + storageDef.chmodMode
                        + " " + storageDef.volume.mountPath + " ; ";
                }

                init.command = {"sh"s,
                                "-c"s,
                                cmd};

                init.image = "busybox";
                init.name = "init-storage-"s + storageDef.volume.name;

                LOG_DEBUG << logName()
                          << "Adding chown/chmod. "
                          << "initContainer " << init.name;

                k8api::VolumeMount vm;
                vm.name = storageDef.volume.name;
                vm.mountPath = storageDef.volume.mountPath;
                init.volumeMounts.push_back(vm);

                podTemplate->spec.initContainers.push_back(move(init));
            }
        }
    }
}

}
