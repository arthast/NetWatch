set(NETWATCH_CLIENT_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../client-common")

if(NOT TARGET netwatch_client_common)
    add_subdirectory("${NETWATCH_CLIENT_COMMON_DIR}" netwatch_client_common)
endif()

add_library(
    netwatch_target_client STATIC
    src/target_client/client/target_client.cpp
    src/target_client/model/target.cpp
)

target_include_directories(
    netwatch_target_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_target_client
    PUBLIC userver::core
           userver::grpc
           netwatch_client_common
           netwatch_target_proto
)
