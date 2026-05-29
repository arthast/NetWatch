add_library(
    netwatch_monitor_application STATIC
    src/checks/runner/check_runner.cpp
)

target_include_directories(
    netwatch_monitor_application
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_monitor_application
    PUBLIC userver::core
           netwatch_monitor_core
)
