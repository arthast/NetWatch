cmake_minimum_required(VERSION 3.12...3.31)
project(api_gateway CXX)

if(NOT TARGET userver::core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
    include(DownloadUserver)

    find_package(
        userver
        COMPONENTS core
                   grpc
        QUIET
    )
    if(NOT userver_FOUND)
        download_userver(TRY_DIR third_party/userver)
    endif()

    userver_setup_environment()
endif()

set(NETWATCH_ALERT_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/alert-client")
set(NETWATCH_MONITOR_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/monitor-client")
set(NETWATCH_NOTIFICATION_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/notification-client")
set(NETWATCH_PROTO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/netwatch-proto")
set(NETWATCH_TARGET_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/target-client")

if(NOT TARGET netwatch_target_proto)
    add_subdirectory("${NETWATCH_PROTO_LIB_DIR}" netwatch_proto)
endif()

if(NOT TARGET netwatch_monitor_client)
    add_subdirectory("${NETWATCH_MONITOR_CLIENT_DIR}" netwatch_monitor_client)
endif()

if(NOT TARGET netwatch_alert_client)
    add_subdirectory("${NETWATCH_ALERT_CLIENT_DIR}" netwatch_alert_client)
endif()

if(NOT TARGET netwatch_notification_client)
    add_subdirectory("${NETWATCH_NOTIFICATION_CLIENT_DIR}" netwatch_notification_client)
endif()

if(NOT TARGET netwatch_target_client)
    add_subdirectory("${NETWATCH_TARGET_CLIENT_DIR}" netwatch_target_client)
endif()

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/alerts/handlers/active_alerts_handler.cpp
    src/alerts/handlers/alerts_handler.cpp
    src/alerts/json/alert_json.cpp
    src/alerts/service/alerts_service.cpp
    src/alerts/service/alerts_service_component.cpp
    src/checks/handlers/manual_check_handler.cpp
    src/checks/handlers/target_checks_handler.cpp
    src/checks/handlers/target_status_handler.cpp
    src/checks/json/check_json.cpp
    src/checks/service/checks_service.cpp
    src/checks/service/checks_service_component.cpp
    src/common/http_response.cpp
    src/common/path_params.cpp
    src/notifications/handlers/notification_delivery_retry_handler.cpp
    src/notifications/handlers/notification_recipient_by_id_handler.cpp
    src/notifications/handlers/notification_deliveries_handler.cpp
    src/notifications/handlers/notification_recipients_handler.cpp
    src/notifications/handlers/notification_test_email_handler.cpp
    src/notifications/json/delivery_json.cpp
    src/notifications/json/recipient_json.cpp
    src/notifications/json/test_email_json.cpp
    src/notifications/service/notifications_service.cpp
    src/notifications/service/notifications_service_component.cpp
    src/targets/handlers/target_by_id_handler.cpp
    src/targets/handlers/targets_handler.cpp
    src/targets/json/target_json.cpp
    src/targets/service/targets_service.cpp
    src/targets/service/targets_service_component.cpp
    src/web/handlers/openapi_handler.cpp
    src/web/handlers/swagger_ui_handler.cpp
)

target_include_directories(
    ${PROJECT_NAME}_objs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core
           userver::grpc
           netwatch_alert_client
           netwatch_monitor_client
           netwatch_notification_client
           netwatch_target_client
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

include(GNUInstallDirs)

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml)

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
