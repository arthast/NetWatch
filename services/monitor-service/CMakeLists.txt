cmake_minimum_required(VERSION 3.12...3.31)
project(monitor_service CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(
    userver
    COMPONENTS core #
               postgresql
    QUIET
)
if(NOT userver_FOUND)
    # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
    download_userver(TRY_DIR third_party/userver)
endif()

userver_setup_environment()

# Common sources

add_library(
        ${PROJECT_NAME}_objs OBJECT
        src/alerts/handlers/active_alerts_handler.cpp
        src/alerts/handlers/alerts_handler.cpp
        src/alerts/json/alert_json.cpp
        src/alerts/model/alert.cpp
        src/alerts/service/alert_service.cpp
        src/alerts/storage/alert_repository.cpp
        src/checks/handlers/manual_check_handler.cpp
        src/checks/handlers/target_checks_handler.cpp
        src/checks/handlers/target_status_handler.cpp
        src/checks/json/check_json.cpp
        src/checks/model/check_result.cpp
        src/checks/runner/check_runner.cpp
        src/checks/scheduler/target_check_scheduler.cpp
        src/checks/service/check_service.cpp
        src/checks/storage/check_repository.cpp
        src/common/http_response.cpp
        src/common/path_params.cpp
        src/targets/handlers/target_by_id_handler.cpp
        src/targets/handlers/targets_handler.cpp
        src/targets/json/target_json.cpp
        src/targets/model/target.cpp
        src/targets/storage/target_repository.cpp
        src/targets/validation/target_validator.cpp
        src/web/handlers/openapi_handler.cpp
        src/web/handlers/swagger_ui_handler.cpp
)

target_include_directories(
        ${PROJECT_NAME}_objs
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core #
           userver::postgresql
)


# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# Unit Tests
add_executable(
    ${PROJECT_NAME}_unittest
    src/targets/validation/target_validator_test.cpp
)
target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver::utest)
add_google_tests(${PROJECT_NAME}_unittest)

# Functional testing
userver_testsuite_add_simple()

# Install
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
