cmake_minimum_required(VERSION 3.14)

project(netwatch CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(
    userver
    COMPONENTS core
               grpc
               kafka
               postgresql
    QUIET
)
if(NOT userver_FOUND)
    download_userver(TRY_DIR third_party/userver)
endif()

userver_setup_environment()

add_subdirectory(services/monitor-service)
add_subdirectory(services/alert-service)
add_subdirectory(services/notification-service)
add_subdirectory(services/target-service)
add_subdirectory(services/api-gateway)
