set(NETWATCH_PROTO_DIR "${CMAKE_CURRENT_LIST_DIR}/../../proto")

userver_add_grpc_library(
    netwatch_target_proto
    SOURCE_PATH "${NETWATCH_PROTO_DIR}"
    PROTOS
        netwatch/target_service.proto
)

userver_add_grpc_library(
    netwatch_monitor_proto
    SOURCE_PATH "${NETWATCH_PROTO_DIR}"
    PROTOS
        netwatch/monitor_service.proto
)

userver_add_grpc_library(
    netwatch_alert_proto
    SOURCE_PATH "${NETWATCH_PROTO_DIR}"
    PROTOS
        netwatch/alert_service.proto
)
