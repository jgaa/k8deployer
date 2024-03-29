cmake_minimum_required(VERSION 3.8)
project (k8deployer VERSION 0.0.26 LANGUAGES CXX)

option(BOOST_ERROR_CODE_HEADER_ONLY "Work-around for another boost issue" ON)
if (BOOST_ERROR_CODE_HEADER_ONLY)
    add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY=1)
endif()

add_definitions(-DK8DEPLOYER_VERSION=\"${CMAKE_PROJECT_VERSION}\")

if(NOT DEFINED USE_BOOST_VERSION)
    set(USE_BOOST_VERSION 1.65)
endif()

find_package(Threads)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost ${USE_BOOST_VERSION} REQUIRED COMPONENTS
    system
    program_options
    date_time
    context
    coroutine
    chrono
    filesystem
    )

include(cmake/external-projects.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}
    include/k8deployer/AppComponent.h
    include/k8deployer/BaseComponent.h
    include/k8deployer/Cluster.h
    include/k8deployer/ClusterRoleBindingComponent.h
    include/k8deployer/ClusterRoleComponent.h
    include/k8deployer/Component.h
    include/k8deployer/Config.h
    include/k8deployer/ConfigMapComponent.h
    include/k8deployer/DaemonSetComponent.h
    include/k8deployer/DataDef.h
    include/k8deployer/DeploymentComponent.h
    include/k8deployer/DnsProvisioner.h
    include/k8deployer/DnsProvisionerVubercool.h
    include/k8deployer/Engine.h
    include/k8deployer/HostPathStorage.h
    include/k8deployer/HttpRequestComponent.h
    include/k8deployer/IngressComponent.h
    include/k8deployer/JobComponent.h
    include/k8deployer/Kubeconfig.h
    include/k8deployer/NamespaceComponent.h
    include/k8deployer/NfsStorage.h
    include/k8deployer/PersistentVolumeComponent.h
    include/k8deployer/RoleBindingComponent.h
    include/k8deployer/RoleComponent.h
    include/k8deployer/SecretComponent.h
    include/k8deployer/ServiceAccountComponent.h
    include/k8deployer/ServiceComponent.h
    include/k8deployer/StatefulSetComponent.h
    include/k8deployer/Storage.h
    include/k8deployer/buildDependencies.h
    include/k8deployer/exprtk_fn.h
    include/k8deployer/k8/k8api.h
    include/k8deployer/logging.h
    include/k8deployer/probe.h
    src/AppComponent.cpp
    src/BaseComponent.cpp
    src/Cluster.cpp
    src/ClusterRoleBindingComponent.cpp
    src/ClusterRoleComponent.cpp
    src/Component.cpp
    src/ConfigMapComponent.cpp
    src/DaemonSetComponent.cpp
    src/DeploymentComponent.cpp
    src/DnsProvisioner.cpp
    src/DnsProvisionerVubercool.cpp
    src/Engine.cpp
    src/HostPathStorage.cpp
    src/HttpRequestComponent.cpp
    src/IngressComponent.cpp
    src/JobComponent.cpp
    src/Kubeconfig.cpp
    src/NamespaceComponent.cpp
    src/NfsStorage.cpp
    src/PersistentVolumeComponent.cpp
    src/RoleBindingComponent.cpp
    src/RoleComponent.cpp
    src/SecretComponent.cpp
    src/ServiceAccountComponent.cpp
    src/ServiceComponent.cpp
    src/StatefulSetComponent.cpp
    src/Storage.cpp
    src/exprtk_fn.cpp
    src/main.cpp
    )

add_dependencies(${PROJECT_NAME} externalRestcCpp externalLogfault externalExprtk)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(RESTC_CPP_LIB restc-cppD)
else()
    set(RESTC_CPP_LIB restc-cpp)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${ExprtkIncludeDir} ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    ${RESTC_CPP_LIB}
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    stdc++fs
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_definitions(
    -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1
    -DBOOST_ALL_DYN_LINK=1
    )
