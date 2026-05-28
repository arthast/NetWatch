add_library(
    netwatch_target_storage STATIC
    src/targets/storage/target_repository.cpp
)

target_include_directories(
    netwatch_target_storage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    netwatch_target_storage
    PUBLIC userver::postgresql
           netwatch_target_domain
)
