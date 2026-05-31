add_library(
    netwatch_monitor_client STATIC
    src/checks/client/check_client.cpp
    src/checks/model/check_result.cpp
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
)
