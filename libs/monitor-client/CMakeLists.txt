set(NETWATCH_CLIENT_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../client-common")

if(NOT TARGET netwatch_client_common)
    add_subdirectory("${NETWATCH_CLIENT_COMMON_DIR}" netwatch_client_common)
endif()

add_library(
    netwatch_monitor_client STATIC
    src/monitor_client/client/check_client.cpp
    src/monitor_client/model/check_result.cpp
)

target_include_directories(
    netwatch_monitor_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_monitor_client
    PUBLIC userver::core
           userver::grpc
           netwatch_client_common
           netwatch_monitor_proto
)
