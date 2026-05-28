add_library(
    netwatch_target_client STATIC
    src/targets/client/target_client.cpp
)

target_include_directories(
    netwatch_target_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_target_client
    PUBLIC userver::core
           userver::grpc
           userver::postgresql
           netwatch_grpc_proto
           netwatch_target_core
)
