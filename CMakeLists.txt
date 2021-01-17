cmake_minimum_required(VERSION 3.8)
project (k8deployer VERSION 0.0.1 LANGUAGES CXX)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS
    system
    program_options
    date_time
    context
    coroutine
    chrono
    log
    )

include(cmake/external-projects.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}
    main.cpp
    AppComponent.cpp
    BaseComponent.cpp
    Cluster.cpp
    Component.cpp
    ConfigMapComponent.cpp
    DeploymentComponent.cpp
    Engine.cpp
    JobComponent.cpp
    NfsStorage.cpp
    PersistentVolumeComponent.cpp
    PortForward.cpp
    SecretComponent.cpp
    ServiceComponent.cpp
    StatefulSetComponent.cpp
    Storage.cpp
    include/k8deployer/AppComponent.h
    include/k8deployer/BaseComponent.h
    include/k8deployer/Cluster.h
    include/k8deployer/Component.h
    include/k8deployer/Config.h
    include/k8deployer/ConfigMapComponent.h
    include/k8deployer/DeploymentComponent.h
    include/k8deployer/Engine.h
    include/k8deployer/JobComponent.h
    include/k8deployer/NfsStorage.h
    include/k8deployer/PortForward.h
    include/k8deployer/SecretComponent.h
    include/k8deployer/ServiceComponent.h
    include/k8deployer/StatefulSetComponent.h
    include/k8deployer/Storage.h
    include/k8deployer/k8/Event.h
    include/k8deployer/k8/k8api.h
    include/k8deployer/logging.h
    include/k8deployer/probe.h
    include/k8deployer/buildDependencies.h
    include/k8deployer/PersistentVolumeComponent.h
    )

add_dependencies(${PROJECT_NAME} externalRestcCpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(RESTC_CPP_LIB restc-cppD)
else()
    set(RESTC_CPP_LIB restc-cpp)
endif()

target_link_libraries(${PROJECT_NAME}
    ${RESTC_CPP_LIB}
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    stdc++fs
    )

add_definitions(
    -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1
    -DBOOST_ALL_DYN_LINK=1
    )
