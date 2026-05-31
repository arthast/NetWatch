add_library(
    netwatch_alert_client STATIC
    src/alerts/client/alert_client.cpp
    src/alerts/model/alert.cpp
)

target_include_directories(
    netwatch_alert_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_alert_client
    PUBLIC userver::core
           userver::grpc
           netwatch_grpc_proto
)
