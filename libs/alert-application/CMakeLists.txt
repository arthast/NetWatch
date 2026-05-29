add_library(
    netwatch_alert_application STATIC
    src/alerts/service/alert_service.cpp
)

target_include_directories(
    netwatch_alert_application
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_alert_application
    PUBLIC netwatch_monitor_core
           netwatch_alert_storage
)
