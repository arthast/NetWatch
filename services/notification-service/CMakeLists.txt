cmake_minimum_required(VERSION 3.12...3.31)
project(notification_service CXX)

if(NOT TARGET userver::core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
    include(DownloadUserver)

    find_package(
        userver
        COMPONENTS core
                   kafka
                   postgresql
        QUIET
    )
    if(NOT userver_FOUND)
        download_userver(TRY_DIR third_party/userver)
    endif()

    userver_setup_environment()
endif()

if(NOT TARGET userver::kafka)
    find_package(userver REQUIRED COMPONENTS kafka)
endif()

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/notifications/events/alert_event.cpp
    src/notifications/repository/notification_repository.cpp
    src/notifications/service/email_provider.cpp
    src/notifications/service/email_delivery_sender.cpp
    src/notifications/service/notification_consumer.cpp
)

target_include_directories(
    ${PROJECT_NAME}_objs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core
           userver::kafka
           userver::postgresql
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

include(GNUInstallDirs)

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

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
