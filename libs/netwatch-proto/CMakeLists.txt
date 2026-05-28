set(NETWATCH_PROTO_DIR "${CMAKE_CURRENT_LIST_DIR}/../../proto")

userver_add_grpc_library(
    netwatch_grpc_proto
    SOURCE_PATH "${NETWATCH_PROTO_DIR}"
    PROTOS
        netwatch/target_service.proto
        netwatch/monitor_service.proto
)
