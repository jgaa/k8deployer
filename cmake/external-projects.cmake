set(EXTERNAL_PROJECTS_PREFIX ${CMAKE_BINARY_DIR}/external-projects)
set(EXTERNAL_PROJECTS_INSTALL_PREFIX ${EXTERNAL_PROJECTS_PREFIX}/installed)

include(GNUInstallDirs)

# MUST be called before any add_executable() # https://stackoverflow.com/a/40554704/8766845
link_directories(${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
include_directories($<BUILD_INTERFACE:${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>)

include(ExternalProject)

ExternalProject_Add(externalRestcCpp
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/jgaa/restc-cpp.git"
    GIT_TAG "k8deployer"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_INSTALL_PREFIX}
        -DRESTC_CPP_WITH_UNIT_TESTS=OFF
        -DRESTC_CPP_WITH_FUNCTIONALT_TESTS=OFF
        -DRESTC_CPP_WITH_EXAMPLES=OFF
        -DRESTC_CPP_USE_CPP17=ON
        -DRESTC_CPP_LOG_WITH_BOOST_LOG=OFF
        -DRESTC_CPP_LOG_WITH_CLOG=ON
        -DRESTC_CPP_LOG_LEVEL_STR=info
        #-DRESTC_CPP_LOG_JSON_SERIALIZATION=1
        -DBOOST_ROOT=${BOOST_ROOT}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        #-DRESTC_CPP_THREADED_CTX=ON
        -DRESTC_BOOST_VERSION=${USE_BOOST_VERSION}
        -DBOOST_ERROR_CODE_HEADER_ONLY=1
    )

ExternalProject_Add(externalLogfault
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/jgaa/logfault.git"
    GIT_TAG "master"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_INSTALL_PREFIX}
    )

ExternalProject_Add(externalExprtk
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/ArashPartow/exprtk.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

ExternalProject_Get_Property(externalExprtk source_dir)
set(ExprtkIncludeDir ${source_dir})
