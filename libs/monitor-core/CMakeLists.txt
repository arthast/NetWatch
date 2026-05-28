add_library(
    netwatch_monitor_core STATIC
    src/alerts/model/alert.cpp
    src/checks/model/check_result.cpp
)

target_include_directories(
    netwatch_monitor_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_monitor_core
    PUBLIC netwatch_target_domain
)
