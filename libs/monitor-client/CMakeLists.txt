add_library(
    netwatch_monitor_client STATIC
    src/alerts/client/alert_client.cpp
    src/checks/client/check_client.cpp
)

target_include_directories(
    netwatch_monitor_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_monitor_client
    PUBLIC userver::core
           userver::grpc
           netwatch_grpc_proto
           netwatch_monitor_core
)
