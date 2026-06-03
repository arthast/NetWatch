add_library(
    netwatch_client_common STATIC
    src/client_common/call_options.cpp
)

target_include_directories(
    netwatch_client_common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_client_common
    PUBLIC userver::grpc
)
