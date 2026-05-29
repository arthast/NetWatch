add_library(
    netwatch_alert_storage STATIC
    src/alerts/storage/alert_repository.cpp
)

target_include_directories(
    netwatch_alert_storage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_alert_storage
    PUBLIC userver::postgresql
           netwatch_monitor_core
)
