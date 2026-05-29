add_library(
    netwatch_monitor_storage STATIC
    src/alerts/storage/alert_repository.cpp
    src/checks/storage/check_repository.cpp
)

target_include_directories(
    netwatch_monitor_storage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_monitor_storage
    PUBLIC userver::postgresql
           netwatch_monitor_core
)
