set(NETWATCH_CLIENT_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../client-common")

if(NOT TARGET netwatch_client_common)
    add_subdirectory("${NETWATCH_CLIENT_COMMON_DIR}" netwatch_client_common)
endif()

add_library(
    netwatch_notification_client STATIC
    src/notification_client/client/notification_client.cpp
    src/notification_client/model/email_recipient.cpp
)

target_include_directories(
    netwatch_notification_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_notification_client
    PUBLIC userver::core
           userver::grpc
           netwatch_client_common
           netwatch_notification_proto
)
