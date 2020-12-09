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
    include/k8deployer/Config.h
    include/k8deployer/logging.h
    PortForward.cpp
    include/k8deployer/PortForward.h
    Engine.cpp
    include/k8deployer/Engine.h
    Cluster.cpp
    include/k8deployer/Cluster.h
    include/k8deployer/k8/Deployment.h
    include/k8deployer/k8/Event.h
    Component.cpp
    include/k8deployer/Component.h
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
