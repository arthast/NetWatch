cmake_minimum_required(VERSION 3.12...3.31)
project(target_service CXX)

if(NOT TARGET userver::core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
    include(DownloadUserver)

    find_package(
        userver
        COMPONENTS core
                   grpc
                   postgresql
        QUIET
    )
    if(NOT userver_FOUND)
        download_userver(TRY_DIR third_party/userver)
    endif()

    userver_setup_environment()
endif()

set(NETWATCH_PROTO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/netwatch-proto")

if(NOT TARGET netwatch_target_proto)
    add_subdirectory("${NETWATCH_PROTO_LIB_DIR}" netwatch_proto)
endif()

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/target_service.cpp
    src/targets/model/target.cpp
    src/targets/storage/target_repository.cpp
    src/targets/validation/target_validator.cpp
)

target_include_directories(
    ${PROJECT_NAME}_objs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core
           userver::grpc
           userver::postgresql
           netwatch_target_proto
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

add_executable(
    netwatch_target_service_unittest
    tests/target_validator_test.cpp
)
target_link_libraries(
    netwatch_target_service_unittest
    PRIVATE ${PROJECT_NAME}_objs
            userver::utest
)
add_google_tests(netwatch_target_service_unittest)

include(GNUInstallDirs)

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml)

install(
    TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)
install(
    FILES ${CONFIGS_FILES}
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)
