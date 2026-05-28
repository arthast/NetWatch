cmake_minimum_required(VERSION 3.12...3.31)
project(api_gateway CXX)

if(NOT TARGET userver::core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../monitor-service/cmake")
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
    src/alerts/client/alert_client.cpp
    src/alerts/handlers/active_alerts_handler.cpp
    src/alerts/handlers/alerts_handler.cpp
    src/alerts/json/alert_json.cpp
    src/checks/client/check_client.cpp
    src/checks/handlers/manual_check_handler.cpp
    src/checks/handlers/target_checks_handler.cpp
    src/checks/handlers/target_status_handler.cpp
    src/checks/json/check_json.cpp
    src/common/http_response.cpp
    src/common/path_params.cpp
    src/targets/handlers/target_by_id_handler.cpp
    src/targets/handlers/targets_handler.cpp
    src/targets/json/target_json.cpp
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
           netwatch_grpc_proto
           netwatch_monitor_core
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
