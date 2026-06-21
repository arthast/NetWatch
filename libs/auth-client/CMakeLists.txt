set(NETWATCH_CLIENT_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../client-common")

if(NOT TARGET netwatch_client_common)
    add_subdirectory("${NETWATCH_CLIENT_COMMON_DIR}" netwatch_client_common)
endif()

add_library(
    netwatch_auth_client STATIC
    src/auth_client/client/auth_client.cpp
    src/auth_client/model/auth.cpp
)

target_include_directories(
    netwatch_auth_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_auth_client
    PUBLIC userver::core
           userver::grpc
           netwatch_client_common
           netwatch_auth_proto
)
