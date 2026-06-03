cmake_minimum_required(VERSION 3.12...3.31)
project(monitor_service CXX)

if(NOT TARGET userver::core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
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

set(NETWATCH_ALERT_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/alert-client")
set(NETWATCH_MONITOR_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/monitor-client")
set(NETWATCH_PROTO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/netwatch-proto")
set(NETWATCH_TARGET_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/target-client")

if(NOT TARGET netwatch_monitor_proto)
    add_subdirectory("${NETWATCH_PROTO_LIB_DIR}" netwatch_proto)
endif()

if(NOT TARGET netwatch_monitor_client)
    add_subdirectory("${NETWATCH_MONITOR_CLIENT_DIR}" netwatch_monitor_client)
endif()

if(NOT TARGET netwatch_alert_client)
    add_subdirectory("${NETWATCH_ALERT_CLIENT_DIR}" netwatch_alert_client)
endif()

if(NOT TARGET netwatch_target_client)
    add_subdirectory("${NETWATCH_TARGET_CLIENT_DIR}" netwatch_target_client)
endif()

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/checks/grpc/check_grpc_service.cpp
    src/checks/alerting/check_alert_notifier.cpp
    src/checks/model/check_result.cpp
    src/checks/repository/check_lease_repository.cpp
    src/checks/repository/check_repository.cpp
    src/checks/runner/check_runner.cpp
    src/checks/scheduler/target_check_scheduler.cpp
    src/checks/service/check_service.cpp
    src/checks/service/check_service_component.cpp
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
           netwatch_alert_client
           netwatch_monitor_client
           netwatch_monitor_proto
           netwatch_target_client
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

include(GNUInstallDirs)

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
