cmake_minimum_required(VERSION 3.12...3.31)
project(monitor_service CXX)

if(NOT TARGET userver::core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(DownloadUserver)

find_package(
    userver
    COMPONENTS core
               grpc
               postgresql
        QUIET
    )
    if(NOT userver_FOUND)
        # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
        download_userver(TRY_DIR third_party/userver)
    endif()

    userver_setup_environment()
endif()

set(NETWATCH_TARGET_DOMAIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/target-domain")
set(NETWATCH_MONITOR_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/monitor-core")
set(NETWATCH_PROTO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/netwatch-proto")
set(NETWATCH_TARGET_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/target-client")

if(NOT TARGET netwatch_target_domain)
    add_subdirectory("${NETWATCH_TARGET_DOMAIN_DIR}" netwatch_target_domain)
endif()

if(NOT TARGET netwatch_monitor_core)
    add_subdirectory("${NETWATCH_MONITOR_CORE_DIR}" netwatch_monitor_core)
endif()

if(NOT TARGET netwatch_grpc_proto)
    add_subdirectory("${NETWATCH_PROTO_LIB_DIR}" netwatch_grpc_proto)
endif()

if(NOT TARGET netwatch_target_client)
    add_subdirectory("${NETWATCH_TARGET_CLIENT_DIR}" netwatch_target_client)
endif()

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/alerts/grpc/alert_grpc_service.cpp
    src/alerts/service/alert_service.cpp
    src/alerts/storage/alert_repository.cpp
    src/checks/grpc/check_grpc_service.cpp
    src/checks/runner/check_runner.cpp
    src/checks/scheduler/target_check_scheduler.cpp
    src/checks/service/check_service.cpp
    src/checks/storage/check_repository.cpp
)

target_include_directories(
    ${PROJECT_NAME}_objs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core
           userver::grpc
           userver::postgresql
           netwatch_target_domain
           netwatch_monitor_core
           netwatch_grpc_proto
           netwatch_target_client
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
    message(STATUS "Set install prefix: $ENV{PREFIX}")
    file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
    set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(
    TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)
install(
    FILES ${CONFIGS_FILES}
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)
