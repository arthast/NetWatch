add_library(
    netwatch_target_domain STATIC
    src/targets/model/target.cpp
    src/targets/validation/target_validator.cpp
)

target_include_directories(
    netwatch_target_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_target_domain
    PUBLIC userver::core
)

add_executable(
    netwatch_target_domain_unittest
    tests/target_validator_test.cpp
)
target_link_libraries(
    netwatch_target_domain_unittest
    PRIVATE netwatch_target_domain
            userver::utest
)
add_google_tests(netwatch_target_domain_unittest)
